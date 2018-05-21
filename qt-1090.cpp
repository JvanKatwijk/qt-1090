#
/*
 *      qt-1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *
 *	qt-1090 Copyright (C) 2018
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Computing
 *
 *	This file is part of the qt-1090
 *
 *    qt-1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    qt-1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with qt1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include        <QMessageBox>
#include        <QFileDialog>
#include	"adsb-constants.h"
#include	"aircraft-handler.h"
#include	"crc-handling.h"
#include	"qt-1090.h"
#include	"icao-cache.h"
#include	"xclose.h"
#include	"syncviewer.h"
#include	"responder.h"

#include	"device-handler.h"
#ifdef	__HAVE_RTLTCP__
#include	"rtl_tcp-handler.h"
#endif
#ifdef	__HAVE_RTLSDR__
#include	"rtlsdr-handler.h"
#endif
#ifdef	__HAVE_SDRPLAY__
#include	"sdrplay-handler.h"
#endif
#ifdef	__HAVE_HACKRF__
#include	"hackrf-handler.h"
#endif
#include	"file-handler.h"

static
void	*readerThreadEntryPoint (void *arg) {
deviceHandler	*theDevice = static_cast <deviceHandler *>(arg);
	theDevice -> startDevice ();
	return NULL;
}


	qt1090::qt1090 (QSettings *qt1090Settings, int32_t freq, bool network):
	                                              QMainWindow (NULL) {
int	i;
	this	-> qt1090Settings	= qt1090Settings;

	setupUi (this);

	this	-> theDevice	= setDevice (freq, network);
	httpPort	= qt1090Settings	-> value ("http_port", 8080). toInt ();
	bitstoShow	= qt1090Settings	-> value ("bitstoShow", 16). toInt ();
	for (i = 0; i < 31; i ++)
	   table [i] = 0;
	viewer		= new syncViewer (dumpview, bitstoShow);
	show_preambles	= false;
	handle_errors	= NO_ERRORFIX;
	check_crc	= true;
	net		= false;
	metric		= false;
	interactive	= false;
	interactive_ttl	= INTERACTIVE_TTL;

//	Allocate the "working" vector
	data_len = DATA_LEN + (FULL_LEN - 1) * 4;
	magnitudeVector	= new uint16_t [data_len / 2];

//	Allocate the ICAO address cache.
	icao_cache	= new icaoCache	();
	aircrafts	= NULL;
	interactive_last_update = 0;

	screenTimer. setInterval (2000);
	connect (&screenTimer, SIGNAL (timeout (void)),
                 this, SLOT (updateScreen (void)));
        screenTimer. start (2000);

/* Statistics */
	stat_valid_preamble	= 0;
	stat_goodcrc		= 0;
	stat_fixed		= 0;
	stat_single_bit_fix	= 0;
	stat_two_bits_fix	= 0;
	stat_http_requests	= 0;
	stat_phase_corrected	= 0;
//
	ttl_selector	-> setValue (INTERACTIVE_TTL);

//	connect the device
	connect (theDevice, SIGNAL (dataAvailable (void)),
	         this, SLOT (processData (void)));
	connect (interactiveButton, SIGNAL (clicked (void)),
	         this, SLOT (handle_interactiveButton (void)));
	connect (errorhandlingCombo, SIGNAL (activated (const QString &)),
	         this, SLOT (handle_errorhandlingCombo (const QString &)));
	connect (httpButton, SIGNAL (clicked (void)),
	         this, SLOT (handle_httpButton (void)));
	connect (ttl_selector, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ttl (int)));
	connect (metricButton, SIGNAL (clicked (void)),
	         this, SLOT (handle_metricButton (void)));
	connect (show_preamblesButton, SIGNAL (clicked (void)),
		 this, SLOT (handle_show_preamblesButton (void)));

	pthread_create (&reader_thread,
	                NULL,
	                readerThreadEntryPoint,
	                (void *)(theDevice));
	avg_corr		= 0;
	correlationCounter	= 0;
//	display the version
        QString v = "qt-1090 -" + QString (CURRENT_VERSION);
        QString versionText = "qt-1090 version: " + QString(CURRENT_VERSION);
        versionText += " Build on: " + QString(__TIMESTAMP__) + QString (" ") + QString (GITHASH);
        versionName     -> setText (v);
        versionName     -> setToolTip (versionText);

}

	qt1090::~qt1090	(void) {
	delete[] magnitudeVector;	
	delete	icao_cache;
	delete	theDevice;
}

void	qt1090::finalize	(void) {
	theDevice	-> stopDevice ();
	screenTimer. stop ();
	pthread_cancel (reader_thread);
	pthread_join (reader_thread, NULL);
}

/* Return -1 if the message is out of fase left-side
 * Return  1 if the message is out of fase right-size
 * Return  0 if the message is not particularly out of phase.
 *
 * Note: this function will access m [-1], so the caller should make sure to
 * call it only if we are not at the start of the current buffer.
 */
int	detectOutOfPhase (uint16_t *m, int offset) {
	if (m [offset + 3 ] > m [offset + 2] / 3) return 1;
	if (m [offset + 10] > m [offset + 9] / 3) return 1;
	if (m [offset + 6 ] > m [offset + 7] / 3) return -1;
	if (m [offset - 1 ] > m [offset + 0] / 3) return -1;
	return 0;
}

/*
 *	This function does not really correct the phase of the message,
 *	it just applies a transformation to the first sample
 *	representing a given bit:
 *
 *	If the previous bit was one, we amplify it a bit.
 *	If the previous bit was zero, we decrease it a bit.
 *
 *	This simple transformation makes the message a bit more likely to be
 *	correctly decoded for out of phase messages:
 *
 *	When messages are out of phase there is more uncertainty in
 *	sequences of the same bit multiple times, since 11111 will be
 *	transmitted as continuously altering magnitude (high, low, high, low...)
 * 
 *	However because the message is out of phase some part of the high
 *	is mixed in the low part, so that it is hard to distinguish if it is
 *	a zero or a one.
 *
 *	However when the message is out of phase, passing from 0 to 1 or from
 *	1 to 0 happens in a very recognizable way, for instance in the 0 -> 1
 *	transition, magnitude goes low, high, high, low, and one of of the
 *	two middle samples the high will be *very* high as part of the previous
 *	or next high signal will be mixed there.
 *
 *	Applying our simple transformation we make more likely if the current
 *	bit is a zero, to detect another zero. Symmetrically if it is a one
 *	it will be more likely to detect a one because of the transformation.
 *	In this way similar levels will be interpreted more likely in the
 *	correct way.
 */
void applyPhaseCorrection (uint16_t *m, int offset) {
int j;

	offset += 16; /* Skip preamble. */
	for (j = 0; j < (LONG_MSG_BITS-1) * 2; j += 2) {
	   if (m [offset + j] > m [offset + j + 1]) {
            /* One */
	      m [offset + j + 2] = (m [offset + j + 2] * 5) / 4;
	   } else {
	    /* Zero */
	      m [offset + j + 2] = (m [offset + j + 2] * 4) / 5;
	   }
	}
}

//	Decode all the next 112 bits, regardless of the actual message
//	size, we do not know the actual size yet.
int	qt1090::decodeBits (uint8_t *bits, uint16_t *m) {
int	errors = 0;
int	i;
int	first, second;

	for (i = 0; i < LONG_MSG_BITS * 2; i += 2) {
	   first	=  m [i];
	   second	=  m [i + 1];

           if (((0.85 * first <= second) && (second <= 1.15 * first)) ||
               ((0.85 * second <= first) && (first <= 1.15 * second))) {
/*
 *	Checking if two adjacent samples have about the same magnitude
 *	is an effective way to detect if it's just random noise
 *	that was detected as a valid preamble.
 */
                bits [i / 2] = 2; /* error */
                if (i < SHORT_MSG_BITS * 2)
                   errors++;
	   } else
	   if (first > second) {
	      bits [i / 2] = 1;
	   } else {
//	(first < second) for exclusion  */
	      bits [i / 2] = 0;
	   }
        }
	return errors;
}

/*
 *	Detect a Mode S messages inside the magnitude buffer pointed
 *	by 'm' and of size 'mlen' bytes.
 *	Every detected Mode S message is convert it into a
 *	stream of bits and passed to the function to display it.
 */
static int correlationVector [] = {
	   20, 1, 20, 1, 1, 1, 1, 20, 1, 20, 1, 1, 1, 1};

static float avgValue	= 0;

void	qt1090::detectModeS (uint16_t *m, uint32_t mlen) {
uint8_t bits	[LONG_MSG_BITS];
uint8_t	msg 	[LONG_MSG_BITS / 8];
uint16_t aux 	[LONG_MSG_BITS * 2];
uint32_t j, k;
double	correlation;
int	high;
bool	phaseCorrected = false;
/*
 *	The Mode S preamble is made of impulses of 0.5 microseconds at
 *	the following time offsets:
 *
 *	0   - 0.5 usec: first impulse.
 *	1.0 - 1.5 usec: second impulse.
 *	3.5 - 4   usec: third impulse.
 *	4.5 - 5   usec: last impulse.
 * 
 *	Since we are sampling at 2 Mhz every sample in our magnitude vector
 *	is 0.5 usec, so the preamble will look like this, assuming there is
 *	an impulse at offset 0 in the array:
 *
 *	0   -----------------
 *	1   -
 *	2   ------------------
 *	3   --
 *	4   -
 *	5   --
 *	6   -
 *	7   ------------------
 *	8   --
 *	9   -------------------
 */
	for (j = 0; j < mlen - FULL_LEN * 2; j++) {
	   int i, errors;
//
//
//	The question here is whether or not to match with a
//	correlation, 
	   correlation = 0;
	   for (k = 0; k < 14; k ++) 
	      correlation += correlationVector [k] * m [j + k];

	   avgValue	= 0.9999 * avgValue + 0.0001 * m[j];
	   avg_corr = 0.99999 * avg_corr + 0.00001 * correlation;
	   if (correlationCounter < 100000) {
	      correlationCounter ++;
	      continue;
	   }

	   if (correlation < 2 * avg_corr)
	      continue;

	   if (!(m [j]   > 1.5 * m [j + 1] &&
	         1.5 * m [j+1] < m [j+2] &&
	         m [j+2] > m [j+3] &&
	         m [j+3] < m [j] &&
	         m [j+4] < m [j] &&
	         m [j+5] < m [j] &&
	         m [j+6] < m [j] &&
	         m [j+7] > m [j+8] &&
	         m [j+8] < m [j+9] &&
	         m [j+9] > m [j+6]))
	      continue;

	   high = (m [j] + m [j + 2] + m [j + 7] + m [j + 9]) / 6;
	   if (m [j + 4] >= high || m [j + 5] >= high)
	      continue;
	   if (m [j + 11] >= high ||
	       m [j + 12] >= high ||
               m [j + 13] >= high ||
	       m [j + 14] >= high)
	      continue;

	
	   errors = decodeBits (bits, &m [j + PREAMBLE_US * 2]);
	   if (errors > 0) {
	      memcpy (aux, &m [j + PREAMBLE_US * 2], sizeof(aux));
              if (detectOutOfPhase (m, j)) {
	         applyPhaseCorrection (m, j);
	         phaseCorrected = true;
	      }
//	Restore the original message after extracting the bits
	      errors = decodeBits (bits, &m [j + PREAMBLE_US * 2]);
	      memcpy (&m[j + PREAMBLE_US * 2], aux, sizeof (aux));
	   }

	   if (errors >= 4)	
	      continue;		// seems hopeless
	   stat_valid_preamble++;	// bold assumption here
	   validPreambles -> display ((int)stat_valid_preamble);
//
//	one way or the other, we are here with bits
//	Pack bits into bytes */
	   for (i = 0; i < LONG_MSG_BITS / 8; i ++) {
	      msg [i] = 0;
	      for (k = 0; k < 8; k ++)
	         msg [i] |= bits [8 * i + k] << (7 - k);
	   }

	   int msgtype	= msg [0] >> 3;
	   int msglen	= messageLenByType (msgtype) / 8;
//
/*	If we reached this point, it is possible that we
 *	have a Mode S message in our hands, but it may still be broken
 *	and CRC may not be correct.
 *	The message is decoded and checked, if false, we give up
 */
	   message mm (handle_errors, icao_cache, msg);

	   if (!mm. is_crcok ()) {
	      if (show_preambles)
                 update_view (&m [j], false);
	      continue;
	   }
//
//	wow, it seems we have something here
	   update_view (&m [j], true);

//	update statistics
	   if (mm. errorbit == -1) {	// no corrections
	      if (mm. is_crcok ()) {
	         stat_goodcrc++;
	         goodCrc	-> display ((int)stat_goodcrc);
	      }
	   }
	   else {
	      stat_fixed++;
	      fixed	-> display ((int)stat_fixed);
	      if (mm. errorbit < LONG_MSG_BITS) {
	         stat_single_bit_fix++;
	         single_bit_fixed -> display ((int)stat_single_bit_fix);
	      }
	      else {
	         stat_two_bits_fix++;
	         two_bit_fixed -> display ((int)stat_two_bits_fix);
	      }
	   }
//
//	prepare for the next round
	   if (mm. is_crcok ()) {
	      j += (PREAMBLE_US + (msglen * 8)) * 2;
	      if (phaseCorrected) {
	         stat_phase_corrected ++;
	         phase_corrected -> display ((int)stat_phase_corrected);
	         phaseCorrected = false;
	      }

	      table [msgtype & 0x1F] ++;	
	      update_table (msgtype & 0x1F, table [msgtype & 0x1F]);
/*
 *	Track aircrafts in interactive mode or if the HTTP
 *	interface is enabled.
 */
	      if (interactive || stat_http_requests > 0) 
	         aircrafts = interactiveReceiveData (aircrafts, &mm);
/*
 *	In non-interactive way, display messages on standard output.
 */
	      if (!interactive) {
	         mm. displayMessage (check_crc);
	         printf ("\n");
	      }
	   }
	}
}

//////////////////////////////////////////////////////////////////////////

//	this slot is called upon the arrival of data
void	qt1090::processData (void) {
int16_t lbuf [DATA_LEN / 2];

	while (theDevice -> Samples () > DATA_LEN / 2) {
	   theDevice -> getSamples (lbuf, DATA_LEN / 2);
	   memcpy (&magnitudeVector [0],
	           &magnitudeVector [DATA_LEN / 2],
	           ((FULL_LEN - 1) * sizeof (uint16_t)) * 4 / 2);
	   memcpy (&magnitudeVector [(FULL_LEN - 1) * 4 / 2],
	           lbuf, DATA_LEN / 2 * sizeof (int16_t));
	   detectModeS	(magnitudeVector, data_len / 2);
	}
}
//
//	Timer driven
void	qt1090::updateScreen (void) {
	   aircrafts	= removeStaleAircrafts (aircrafts, interactive_ttl);
	   if (interactive) 
	      showPlanes (aircrafts, metric);
}
//
///////////////////////////////////////////////////////////////////////

void	qt1090::update_view (uint16_t *m, bool flag) {
	viewer -> Display (m, flag);
}

#include <QCloseEvent>
void	qt1090::closeEvent (QCloseEvent *event) {
        QMessageBox::StandardButton resultButton =
                        QMessageBox::question (this, "qt1090",
                                               tr("Are you sure?\n"),
                                               QMessageBox::No | QMessageBox::Yes,
                                               QMessageBox::Yes);
        if (resultButton != QMessageBox::Yes) {
           event -> ignore();
        } else {
           finalize ();
           event -> accept ();
        }
}

void	qt1090::handle_interactiveButton (void) {
	interactive = !interactive;
}

void	qt1090::handle_httpButton (void) {
	net	= !net;
	if (net) {
	   httpServer		= new QHttpServer (this);
	   connect	(httpServer,
	                 SIGNAL (newRequest (QHttpRequest*, QHttpResponse*)),
	                 this,
	                 SLOT (handleRequest (QHttpRequest*, QHttpResponse*)));
           httpServer -> listen (QHostAddress::Any, httpPort);
	   QString text = "port ";
	   text. append (QString:: number (httpPort));
	   
	   stat_http_requests	= 0;
	   httpPortLabel -> setText (text);
	}
	else 
	if (httpServer != NULL) {
	   disconnect	(httpServer,
	                 SIGNAL (newRequest(QHttpRequest*, QHttpResponse*)),
	                 this,
	                 SLOT (handleRequest(QHttpRequest*, QHttpResponse*)));
	   delete httpServer;
	   httpPortLabel	-> setText ("   ");
	   httpServer = NULL;
	   stat_http_requests	= 0;
	}

	httpButton -> setText (net ? "http on" : "http off");
}

void	qt1090::set_ttl	(int l) {
	interactive_ttl	= l;
}

void	qt1090::handle_metricButton (void) {
	metric = !metric;
	metricButton	-> setText (metric ? "metric" : "not metric");
}

void	qt1090::handle_errorhandlingCombo (const QString &s) {
	if (s == "no correction")
	   handle_errors =  NO_ERRORFIX;
	else
	if (s == "normal correction")
	   handle_errors = NORMAL_ERRORFIX;
	else
	   handle_errors = STRONG_ERRORFIX;
}

void	qt1090::update_table	(int16_t index, int newval) {
	switch (index) {
	   case 0:
	      DF0	-> display (newval);
	      break;
	   case 4:
	      DF4	-> display (newval);
	      break;
	   case 5:
	      DF5	-> display (newval);
	      break;
	   case 11:
	      DF11	-> display (newval);
	      break;
	   case 16:
	      DF16	-> display (newval);
	      break;
	   case 17:
	      DF17	-> display (newval);
	      break;
	   case 20:
	      DF20	-> display (newval);
	      break;
	   case 21:
	      DF21	-> display (newval);
	      break;
	   default:
	      fprintf (stderr, "DF%d -> %d\n", index, newval);
	      break;
	}
}

void	qt1090::handle_show_preamblesButton (void) {
	show_preambles	= !show_preambles;
}

void    qt1090::handleRequest (QHttpRequest *req,
	                       QHttpResponse *resp) {
        if (req -> methodString () != "HTTP_GET")
	   return;
	
	stat_http_requests ++;
        (void)new Responder (req, resp, this -> aircrafts);
}
//
//
//	selecting a device
deviceHandler	*qt1090::setDevice (int freq, bool network) {
deviceHandler	*inputDevice	= NULL;

#ifdef	__HAVE_RTLTCP__
	if (network) {
	   try {
	      inputDevice = new rtltcpHandler (qt1090Settings, freq);
	      return inputDevice;
	   } catch (int e) {
	      fprintf (stderr, "no valid network connection, trying devices\n");
	   }
	}
#endif
	
#ifdef	__HAVE_HACKRF__
	try {
	   inputDevice	= new hackrfHandler (qt1090Settings, freq);
	   return inputDevice;
	} catch (int e) {
	   fprintf (stderr, "no hackrf detected, going on\n");
	}
#endif
#ifdef	__HAVE_SDRPLAY__
	try {
	   inputDevice	= new sdrplayHandler (qt1090Settings, freq);
	   return inputDevice;
	} catch (int e) {
	   fprintf (stderr, "no sdrplay detected, going on\n");}
#endif
#ifdef	__HAVE_RTLSDR__
	try {
	   inputDevice	= new rtlsdrHandler (qt1090Settings, freq);
	   return inputDevice;
	} catch (int e) {
	   fprintf (stderr, "no rtlsdr device detected, going on\n");
	}
#endif
//
//	if everything fails, the user probably meant to read from
//	a file
	QString file = QFileDialog::getOpenFileName (this,
	                                                tr ("Open file ..."),
	                                                QDir::homePath (),
	                                                tr ("iq data (*.iq)"));
	file		= QDir::toNativeSeparators (file);
	try {
	   inputDevice	= new fileHandler (file, true);
	}
	catch (int e) {
	   QMessageBox::warning (this, tr ("Warning"),
	                               tr ("file not found"));
	   return NULL;
	}
	
	return new  deviceHandler ();
}

