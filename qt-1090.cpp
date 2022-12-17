#
/*
 *      qt-1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *	
 *	The demodulation code is based on
 *	demod_2400.c: 2.4MHz Mode S demodulator.
 *	 Copyright (c) 2014,2015 Oliver Jowett <oliver@mutability.co.uk>
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

#include	"device-handler.h"
#ifdef	__HAVE_RTLTCP__
#include	"rtl_tcp-handler.h"
#endif
#ifdef	__HAVE_RTLSDR__
#include	"rtlsdr-handler.h"
#endif
#ifdef	__HAVE_SDRPLAY_V2
#include	"sdrplay-handler-v2.h"
#endif
#ifdef	__HAVE_SDRPLAY_V3
#include	"sdrplay-handler-v3.h"
#endif
#ifdef	__HAVE_HACKRF__
#include	"hackrf-handler.h"
#endif
#include	"file-handler.h"

static
void	*readerThreadEntryPoint (void *arg) {
deviceHandler	*theDevice = static_cast <deviceHandler *>(arg);
	theDevice -> startDevice ();
	return nullptr;
}

		qt1090::qt1090 (QSettings *qt1090Settings,
	                        int32_t freq, bool network):
	                                      QMainWindow (nullptr) {
int	i;
	this	-> qt1090Settings	= qt1090Settings;
	this	-> frequency		= freq;
	this	-> network		= network;
	setupUi (this);
	this	-> viewer	= new spectrumViewer (plotgrid);
	httpPort	= qt1090Settings -> value ("http_port", 8080). toInt ();
	bitstoShow      = qt1090Settings -> value ("bitstoShow", 16). toInt ();

#ifdef	__HAVE_RTLTCP__
	deviceSelecctor	-> addItem ("rtl_tcp");
#endif
#ifdef	__HAVE_RTLSDR__
	deviceSelector	-> addItem ("rtlsdr");
#endif
#ifdef	__HAVE_SDRPLAY_V2
	deviceSelector	-> addItem ("sdrplay-v2");
#endif
#ifdef	__HAVE_SDRPLAY_V3
	deviceSelector	-> addItem ("sdrplay-v3");
#endif
#ifdef	__HAVE_HACKRF__
	deviceSelector	-> addItem ("hackrf");
#endif
#ifdef	__HAVE_PLUTO__
	deviceSelector	-> addItem ("pluto"
#endif
//#include	"file-handler.h"

	for (i = 0; i < 31; i ++)
	   table [i] = 0;
	handle_errors	= NO_ERRORFIX;
	check_crc	= true;
	net		= false;
	httpServer	= nullptr;
	metric		= false;
	interactive	= false;
	handle_interactiveButton ();
	interactive_ttl	= INTERACTIVE_TTL;
	dumpfilePointer	= nullptr;

//	Allocate the "working" vector
	data_len	= DATA_LEN + (FULL_LEN - 1) * 4;
	magnitudeVector	= new uint16_t [data_len / 2 + 10];

//	Allocate the ICAO address cache.
	icao_cache	= new icaoCache	();
	planeList	= nullptr;
	interactive_last_update = 0;

	screenTimer. setInterval (2000);
	connect (&screenTimer, SIGNAL (timeout ()),
                 this, SLOT (updateScreen ()));
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
	connect (interactiveButton, SIGNAL (clicked ()),
	         this, SLOT (handle_interactiveButton ()));
	connect (errorhandlingCombo, SIGNAL (activated (const QString &)),
	         this, SLOT (handle_errorhandlingCombo (const QString &)));
	connect (httpButton, SIGNAL (clicked ()),
	         this, SLOT (handle_httpButton ()));
	connect (ttl_selector, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ttl (int)));
	connect (metricButton, SIGNAL (clicked ()),
	         this, SLOT (handle_metricButton ()));
	connect (dumpButton, SIGNAL (clicked ()),
	         this, SLOT (handle_dumpButton ()));
	connect (deviceSelector, SIGNAL (activated (const QString &)),
	         this, SLOT (setDevice (const QString &)));
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
	net	= false;
	if (httpServer != nullptr) {
	   httpServer	-> stop ();
	   delete httpServer;
	   fprintf (stderr, "httpServer is now quiet\n");
	   httpPortLabel	-> setText ("   ");
	   httpServer = nullptr;
	   stat_http_requests	= 0;
	}

	screenTimer. stop ();
	if (theDevice != nullptr) {
	   fprintf (stderr, "going to stop the input\n");
	   theDevice	-> stopDevice ();
	   pthread_cancel (reader_thread);
	   pthread_join (reader_thread, nullptr);
	   fprintf (stderr, "reader is quiet\n");
	}
}

/*
 *	Detect a Mode S messages inside the magnitude buffer pointed
 *	by 'm' and of size 'mlen' bytes.
 *	Every detected Mode S message is convert it into a
 *	stream of bits and passed to the function to display it.
 */


static inline int slice_phase0(uint16_t *m) {
    return 5 * m[0] - 3 * m[1] - 2 * m[2];
}
static inline int slice_phase1(uint16_t *m) {
    return 4 * m[0] - m[1] - 3 * m[2];
}
static inline int slice_phase2(uint16_t *m) {
    return 3 * m[0] + m[1] - 4 * m[2];
}
static inline int slice_phase3(uint16_t *m) {
    return 2 * m[0] + 3 * m[1] - 5 * m[2];
}
static inline int slice_phase4(uint16_t *m) {
    return m[0] + 5 * m[1] - 5 * m[2] - m[3];
}

//	2.4MHz sampling rate version
//
//	When sampling at 2.4MHz we have exactly 6 samples per 5 symbols.
//	Each symbol is 500ns wide, each sample is 416.7ns wide
//
//	We maintain a phase offset that is expressed in units of
//	1/5 of a sample i.e. 1/6 of a symbol, 83.333ns
//	Each symbol we process advances the phase offset by
//	6 i.e. 6/5 of a sample, 500ns
//
//	The correlation functions above correlate a 1-0 pair
//	of symbols (i.e. manchester encoded 1 bit)
//	starting at the given sample, and assuming that
//	the symbol starts at a fixed 0-5 phase offset within
//	m[0].
//	They return a correlation value, generally
//	interpreted as >0 = 1 bit, <0 = 0 bit
//
//	TODO check if there are better (or more balanced)
//	correlation functions to use here
//

void	qt1090::detectModeS (uint16_t *m, uint32_t mlen) {
uint8_t	msg 	[LONG_MSG_BITS / 8];

uint32_t j;
/*
 *	The Mode S preamble is made of impulses of 0.5 microseconds at
 *	the following time offsets:
 *
 *	0   - 0.5 usec: first impulse.
 *	1.0 - 1.5 usec: second impulse.
 *	3.5 - 4   usec: third impulse.
 *	4.5 - 5   usec: last impulse.
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
	for (j = 0; j < mlen - 20; j++) {
	   uint16_t *preamble = &m [j];
	   int high;
	   uint32_t base_signal, base_noise;
	   int try_phase;

//	Look for a message starting at around sample 0 with phase offset 3..7
//	Ideal sample values for preambles with different phase
//	Xn is the first data symbol with phase offset N
//
//	sample#: 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
//	phase 3: 2/4\0/5\1 0 0 0 0/5\1/3 3\0 0 0 0 0 0 X4
//	phase 4: 1/5\0/4\2 0 0 0 0/4\2 2/4\0 0 0 0 0 0 0 X0
//	phase 5: 0/5\1/3 3\0 0 0 0/3 3\1/5\0 0 0 0 0 0 0 X1
//	phase 6: 0/4\2 2/4\0 0 0 0 2/4\0/5\1 0 0 0 0 0 0 X2
//	phase 7: 0/3 3\1/5\0 0 0 0 1/5\0/4\2 0 0 0 0 0 0 X3
//
        
//	quick check: we must have a rising edge 0->1 and a falling edge 12->13
	   if (!(preamble [0]  < preamble [1] &&
	         preamble [12] > preamble [13]) )
              continue;

	   if (preamble [1] > preamble [2] &&                   // 1
	       preamble [2] < preamble [3] &&
	                        preamble [3] > preamble [4] &&  // 3
	       preamble [8] < preamble [9] &&
	                        preamble[9] > preamble[10] &&   // 9
	       preamble [10] < preamble [11]) {                 // 11-12
//	peaks at 1,3,9,11-12: phase 3
	      high = (preamble [1] + preamble [3] +
	              preamble[9] + preamble [11] + preamble[12]) / 4;
	      base_signal	= preamble[1] + preamble[3] + preamble[9];
	      base_noise	= preamble[5] + preamble[6] + preamble[7];
	   } else
	   if (preamble [1] > preamble[2] &&                       // 1
	       preamble [2] < preamble[3] &&
	                   preamble[3] > preamble[4] &&  // 3
	       preamble [8] < preamble[9] &&
	                   preamble[9] > preamble[10] && // 9
	       preamble [11] < preamble [12]) {                     // 12
//	peaks at 1,3,9,12: phase 4
	      high = (preamble [1] + preamble [3] +
	                 preamble [9] + preamble [12]) / 4;
	      base_signal = preamble [1] + preamble [3] +
	                          preamble [9] + preamble [12];
	      base_noise  = preamble [5] + preamble [6] +
	                          preamble [7] + preamble [8];
	   } else
	   if (preamble [1] > preamble [2] &&                        // 1
	       preamble [2] < preamble [3] &&
	                          preamble[4] > preamble[5] && // 3-4
	       preamble [8] < preamble [9] &&
	                          preamble[10] > preamble[11] && // 9-10
	       preamble [11] < preamble[12]) {                    // 12
//	peaks at 1,3-4,9-10,12: phase 5
	      high = (preamble [1] + preamble [3] +
	              preamble [4] + preamble [9] +
	                      preamble [10] + preamble [12]) / 4;
	      base_signal	= preamble [1] + preamble [12];
	      base_noise	= preamble [6] + preamble [7];
	   } else
	   if (preamble[1] > preamble[2] &&            // 1
	       preamble[3] < preamble[4] &&
	       preamble[4] > preamble[5] &&            // 4
	       preamble[9] < preamble[10] &&
	       preamble[10] > preamble[11] &&          // 10
	       preamble[11] < preamble[12]) {          // 12
//	peaks at 1,4,10,12: phase 6
	      high = (preamble[1] + preamble[4] +
	                      preamble [10] + preamble [12]) / 4;
	      base_signal = preamble [1] + preamble [4] +
	                      preamble [10] + preamble [12];
	      base_noise = preamble [5] + preamble [6] +
	                      preamble [7] + preamble [8];
	   } else
	   if (preamble [2] > preamble [3] &&            // 1-2
	       preamble [3] < preamble [4] &&
	               preamble [4] > preamble [5] &&    // 4
	       preamble [9] < preamble [10] &&
	               preamble[10] > preamble[11] &&    // 10
	       preamble [11] < preamble [12]) {          // 12
//	peaks at 1-2,4,10,12: phase 7
	      high = (preamble [1] + preamble [2] +
	                preamble [4] + preamble [10] + preamble [12]) / 4;
	      base_signal = preamble [4] + preamble [10] + preamble [12];
	      base_noise = preamble[6] + preamble[7] + preamble[8];
	   } else {
//	no suitable peaks
	      continue;
	   }

// Check for enough signal
	   if (base_signal * 2 < 8 * base_noise) // about 3.5dB SNR
	      continue;

//	Check that the "quiet" bits 6,7,15,16,17 are actually quiet
	   if (preamble [5] >= high / 2 ||
               preamble [6] >= high / 2 ||
               preamble [7] >= high / 2 ||
               preamble [8] >= high / 2 ||
               preamble [14] >= high / 2 ||
               preamble [15] >= high / 2 ||
               preamble [16] >= high / 2 ||
               preamble [17] >= high / 2 ||
               preamble [18] >= high / 2) {
	      continue;
	   }
	
//	try phases until we find a good crc (if any)

	   validPreambles -> display ((int)stat_valid_preamble ++);
	   for (try_phase = 4; try_phase <= 8; ++try_phase) {
	   int msgType;
	   uint16_t *pPtr;
           int phase, i, bytelen;

	      pPtr = &m [j + 19] + (try_phase / 5);
	      phase = try_phase % 5;

	      bytelen = LONG_MSG_BITS / 8;
	      for (i = 0; i < bytelen; ++i) {
              uint8_t theByte = 0;

                 switch (phase) {
	            case 0:
	               theByte = 
	                  (slice_phase0 (pPtr) > 0 ? 0x80 : 0) |
	                  (slice_phase2 (pPtr+2) > 0 ? 0x40 : 0) |
	                  (slice_phase4 (pPtr+4) > 0 ? 0x20 : 0) |
	                  (slice_phase1 (pPtr+7) > 0 ? 0x10 : 0) |
	                  (slice_phase3 (pPtr+9) > 0 ? 0x08 : 0) |
	                  (slice_phase0 (pPtr+12) > 0 ? 0x04 : 0) |
	                  (slice_phase2 (pPtr+14) > 0 ? 0x02 : 0) |
	                  (slice_phase4 (pPtr+16) > 0 ? 0x01 : 0);

	                phase = 1;
	                pPtr += 19;
	                break;
                    
	             case 1:
	                theByte =
	                   (slice_phase1 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase3 (pPtr+2) > 0 ? 0x40 : 0) |
	                   (slice_phase0 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase2 (pPtr+7) > 0 ? 0x10 : 0) |
	                   (slice_phase4 (pPtr+9) > 0 ? 0x08 : 0) |
	                   (slice_phase1 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase3 (pPtr+14) > 0 ? 0x02 : 0) |
	                   (slice_phase0 (pPtr+17) > 0 ? 0x01 : 0);

	                phase = 2;
	                pPtr += 19;
	                break;

	             case 2:
	                theByte =
	                   (slice_phase2 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase4 (pPtr+2) > 0 ? 0x40 : 0) |
	                   (slice_phase1 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase3 (pPtr+7) > 0 ? 0x10 : 0) |
	                   (slice_phase0 (pPtr+10) > 0 ? 0x08 : 0) |
	                   (slice_phase2 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase4 (pPtr+14) > 0 ? 0x02 : 0) |
	                   (slice_phase1 (pPtr+17) > 0 ? 0x01 : 0);

	                phase = 3;
	                pPtr += 19;
	                break;

	             case 3:
	               theByte = 
	                   (slice_phase3 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase0 (pPtr+3) > 0 ? 0x40 : 0) |
	                   (slice_phase2 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase4 (pPtr+7) > 0 ? 0x10 : 0) |
	                   (slice_phase1 (pPtr+10) > 0 ? 0x08 : 0) |
	                   (slice_phase3 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase0 (pPtr+15) > 0 ? 0x02 : 0) |
	                   (slice_phase2 (pPtr+17) > 0 ? 0x01 : 0);

	               phase = 4;
	               pPtr += 19;
	               break;

	            case 4:
	               theByte = 
	                   (slice_phase4 (pPtr) > 0 ? 0x80 : 0) |
	                   (slice_phase1 (pPtr+3) > 0 ? 0x40 : 0) |
	                   (slice_phase3 (pPtr+5) > 0 ? 0x20 : 0) |
	                   (slice_phase0 (pPtr+8) > 0 ? 0x10 : 0) |
	                   (slice_phase2 (pPtr+10) > 0 ? 0x08 : 0) |
	                   (slice_phase4 (pPtr+12) > 0 ? 0x04 : 0) |
	                   (slice_phase1 (pPtr+15) > 0 ? 0x02 : 0) |
	                   (slice_phase3 (pPtr+17) > 0 ? 0x01 : 0);

	               phase = 0;
	               pPtr += 20;
	               break;
	         }

	         msg [i] = theByte;
	         if (i == 0) {
	            msgType = msg [0] >> 3;
	            switch (msgType) {
	               case 0: case 4: case 5: case 11:
	                  bytelen = SHORT_MSG_BITS / 8;
	                  break;
                        
	               case 16: case 17: case 18: case 20: case 21: case 24:
	                  break;

	               default:
	                  bytelen = 1; // unknown DF, give up immediately
	                  break;
	            }
	         }
	      }
	      if (bytelen == 1)
	         continue;
//
/*	If we reached this point, it is possible that we
 *	have a Mode S message in our hands, but it may still be broken
 *	and CRC may not be correct.
 *	The message is decoded and checked, if false, we give up
 */
	      message mm (handle_errors, icao_cache, msg);

	      if (mm. is_crcok ()) {
//	update statistics
	         if (mm. errorbit == -1) {    // no corrections
	            if (mm. is_crcok ()) {
	               stat_goodcrc++;
	               goodCrc        -> display ((int)stat_goodcrc);
	            }
	         }
	         else {
	            stat_fixed++;
	            fixed     -> display ((int)stat_fixed);
	            if (mm. errorbit < LONG_MSG_BITS) {
	               stat_single_bit_fix++;
	               single_bit_fixed -> display ((int)stat_single_bit_fix);
	            }
	            else {
	               stat_two_bits_fix++;
	               two_bit_fixed -> display ((int)stat_two_bits_fix);
	            }
	         }

	         j += (PREAMBLE_US  + bytelen * 8) * 2 * 8 * 6/5;
	         table [msgType & 0x1F] ++;	
	         update_table (msgType & 0x1F, table [msgType & 0x1F]);
	         phase_corrected -> display ((int)try_phase);
	         
/*
 *	Track aircrafts in interactive mode or if the HTTP
 *	interface is enabled.
 */
	         if (interactive || stat_http_requests > 0) 
	            planeList = interactiveReceiveData (planeList, &mm);
/*
 *	In non-interactive way, display messages on standard output.
 */
	         if (!interactive) {
	            mm. displayMessage (check_crc);
	            printf ("\n");
	         }
	         break;
	      }
	   }
        }
}

//////////////////////////////////////////////////////////////////////////
static inline
uint16_t jan_abs (std::complex<float> s) {
int16_t  res	= real (s) < 0 ? - real (s) : real (s);
	res += imag (s) < 0  ? - imag (s) : imag (s);
	return res;
}

//	this slot is called upon the arrival of data
void	qt1090::processData (void) {
std::complex<float> lbuf [DATA_LEN / 2];
static int flipper	= 0;

	while (theDevice -> Samples () > DATA_LEN / 2) {
	   theDevice -> getSamples (lbuf, DATA_LEN / 2);
	   memcpy (&magnitudeVector [0],
	           &magnitudeVector [DATA_LEN / 2],
	           ((FULL_LEN - 1) * sizeof (uint16_t)) * 4 / 2);
	   for (int i = 0; i < DATA_LEN / 2; i ++)
	      magnitudeVector [(FULL_LEN - 1) * 4 /  2 + i] =
	                               (int16_t) (jan_abs (lbuf [i]));
	   detectModeS	(magnitudeVector, data_len / 2);
	   flipper ++;
	   if (flipper >= 4) {
	      viewer -> Display (lbuf, DATA_LEN / 2,
	                          amplitudeSlider -> value ());
	      flipper = 0;
	   }
	}
}
//
//	Timer driven
void	qt1090::updateScreen (void) {
	   planeList	= removeStaleAircrafts (planeList,
	                                        interactive_ttl,
	                                        dumpfilePointer);
	   if (interactive) 
	      showPlanes (planeList, metric);
}
//
///////////////////////////////////////////////////////////////////////

#include <QCloseEvent>
void	qt1090::closeEvent (QCloseEvent *event) {
        QMessageBox::StandardButton resultButton =
                        QMessageBox::question (this, "qt1090",
                                               tr ("Are you sure?\n"),
                                               QMessageBox::No | QMessageBox::Yes,
                                               QMessageBox::Yes);
        if (resultButton != QMessageBox::Yes) {
           event -> ignore ();
        } else {
           finalize ();
           event -> accept ();
        }
}

void	qt1090::handle_interactiveButton (void) {
	interactive = !interactive;
	if (interactive)
	   interactiveButton -> setText ("Streaming");
	else
	   interactiveButton	-> setText ("plane list");
}

void	qt1090::handle_httpButton (void) {
	net	= !net;
	if (net) {
	   httpServer	= new httpHandler (this, "./gmap.html");
	   httpServer -> start ();
	   QString text = "port ";
	   text. append (QString:: number (httpPort));
	   stat_http_requests	= 0;
	   httpPortLabel -> setText (text);
	   httpButton	-> setText ("http on");
	}
	else 
	if (httpServer != nullptr) {
	   httpServer	-> stop ();
	   httpPortLabel	-> setText ("   ");
	   httpButton	-> setText ("http off");
	   stat_http_requests	= 0;
	   delete httpServer;
	   httpServer	= nullptr;
	}
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
	if (s == "1 bit correction")
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
	   case 24:
	      DF24	-> display (newval);
	      break;
	   default:
//	      fprintf (stderr, "DF%d -> %d\n", index, newval);
	      break;
	}
}

//	selecting a device
void	qt1090::setDevice (const QString &device) {
theDevice	= nullptr;

#ifdef	__HAVE_RTLTCP__
	if (device = "network") {
	   try {
	      theDevice = new rtltcpHandler (qt1090Settings, this -> frequency);
	   } catch (int e) {
	      fprintf (stderr, "no valid network connection, trying devices\n");
	      return;
	   }
	}
	else
#endif
	
#ifdef	__HAVE_HACKRF__
	if  (device == "hackrf") {
	   try {
	      theDevice	= new hackrfHandler (qt1090Settings, this -> frequency);
	   } catch (int e) {
	      fprintf (stderr, "no hackrf detected, try again\n");
	      return;
	   }
	}
	else
#endif
#ifdef	__HAVE_SDRPLAY_V2
	if (device == "sdrplay-v2") {
	   try {
	      theDevice	= new sdrplayHandler_v2 (qt1090Settings, this -> frequency);
	   } catch (int e) {
	      fprintf (stderr, "no sdrplay detected, try again\n");
	      return;
	   }
	}
	else
#endif
#ifdef	__HAVE_SDRPLAY_V3
	if (device == "sdrplay-v3") {
	   try {
	      theDevice	= new sdrplayHandler_v3 (qt1090Settings, this -> frequency);
	   } catch (int e) {
	      fprintf (stderr, "no sdrplay detected, try again\n");
	      return;
	   }
	}
	else
#endif
#ifdef	__HAVE_RTLSDR__
	if (device == "rtlsdr") {
	   try {
	      theDevice	= new rtlsdrHandler (qt1090Settings, this -> frequency);
	   } catch (int e) {
	      fprintf (stderr, "no rtlsdr device detected, try again\n");
	      return;
	   }
	}
#endif
	if (theDevice == nullptr)
	   return;
//
//	we have a device, 
	disconnect (deviceSelector, SIGNAL (activated (const QString &)),
	            this, SLOT (setDevice (const QString &)));
	deviceSelector	-> hide ();

	connect (theDevice, SIGNAL (dataAvailable ()),
	         this, SLOT (processData ()));
	pthread_create (&reader_thread,
	                nullptr,
	                readerThreadEntryPoint,
	                (void *)(theDevice));
}

void	qt1090::handle_dumpButton () {

	if (dumpfilePointer != 0) {
	   fclose (dumpfilePointer);
	   dumpfilePointer	= nullptr;
	   dumpButton	-> setText ("dump");
	   return;
	}

	QString file = QFileDialog::getSaveFileName (this,
	                                             tr ("Open file ..."),
	                                             QDir::homePath (),
	                                             tr ("txt data (*.txt)"));
	file		= QDir::toNativeSeparators (file);
	dumpfilePointer	= fopen (file. toLatin1 (). data (), "w");
	if (dumpfilePointer != nullptr)
	   dumpButton	-> setText ("dumping");
}

