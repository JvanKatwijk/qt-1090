#
/*
 *      qt-1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *
 *	Copyright (C) 2018
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Computing
 *
 *	This file is part of the qt-1090
 *
 *    qt1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    qt1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with qt1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"device-handler.h"
#include	"anet.h"
#include	"adsb-constants.h"
#include	"aircraft-handler.h"
#include	"crc-handling.h"
#include	"qt-1090.h"
#include	"icao-cache.h"
#include	"xclose.h"
#include	"syncviewer.h"

static
void	*readerThreadEntryPoint (void *arg) {
deviceHandler	*device = static_cast <deviceHandler *>(arg);
	device -> startDevice ();
	return NULL;
}


	qt1090::qt1090 (QSettings *dumpSettings, deviceHandler *device):
	                                      QMainWindow (NULL) {
int	i;
	this	-> device	= device;
	this	-> dumpSettings	= dumpSettings;

	setupUi (this);

	tableWidget	= new QTableWidget (0, 1);
	messageWidget	-> setWidget (tableWidget);
	for (i = 0; i < 31; i ++) {
	   tableWidget	-> insertRow (i);
	   QTableWidgetItem *item0 = new QTableWidgetItem (QString ("    "));
	   item0     -> setTextAlignment (Qt::AlignRight |Qt::AlignVCenter);
	   tableWidget     -> setItem (i, i, item0);
	}

	for (i = 0; i < 32; i ++)
	   table [i] = 0;
	viewer		= new syncViewer (dumpview, 128);
	show_preambles	= false;
	handle_errors	= NO_ERRORFIX;
	check_crc	= true;
	net		= false;
	metric		= false;
	interactive	= false;
	interactive_ttl	= MODES_INTERACTIVE_TTL;

//	Allocate the "working" vector
	data_len = MODES_DATA_LEN + (MODES_FULL_LEN - 1) * 4;
	magnitudeVector	= new uint16_t [data_len / 2];

//	Allocate the ICAO address cache.
	icao_cache	= new icaoCache	();
	aircrafts	= NULL;
	interactive_last_update = 0;

/* Statistics */
	stat_valid_preamble	= 0;
	stat_demodulated	= 0;
	stat_goodcrc		= 0;
	stat_fixed		= 0;
	stat_single_bit_fix	= 0;
	stat_two_bits_fix	= 0;
	stat_http_requests	= 0;
//	stat_sbs_connections	= 0;
	stat_out_of_phase	= 0;
	stat_phase_corrected	= 0;
//
	ttl_selector	-> setValue (MODES_INTERACTIVE_TTL);
//	connect the device
	connect (device, SIGNAL (dataAvailable (void)),
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
	                (void *)(device));
	serverInit ();		// init http server
	avg_corr		= 0;
	correlationCounter	= 0;
}

	qt1090::~qt1090	(void) {
	delete[] magnitudeVector;	
	delete	icao_cache;
}

void	qt1090::finalize	(void) {
	device	-> stopDevice ();
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
int	detectOutOfPhase (uint16_t *m) {
	if (m [3] > m [2] / 3) return 1;
	if (m [10] > m [9] / 3) return 1;
	if (m [6] > m [7]/3) return -1;
	if (m [-1] > m[1]/3) return -1;
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
 *	However when the message is out of phase passing from 0 to 1 or from
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
void applyPhaseCorrection (uint16_t *m) {
int j;

	m += 16; /* Skip preamble. */
	for (j = 0; j < (MODES_LONG_MSG_BITS-1) * 2; j += 2) {
	   if (m [j] > m [j + 1]) {
            /* One */
	      m [j + 2] = (m [j + 2] * 5) / 4;
	   } else {
	    /* Zero */
	      m [j + 2] = (m [j + 2] * 4) / 5;
	   }
	}
}

//	Decode all the next 112 bits, regardless of the actual message
//	size. We'll check the actual message type later. */
int	qt1090::decodeBits (uint8_t *bits, uint16_t *m) {
int	errors = 0;
int	i;
int	delta, first, second;

	for (i = 0; i < MODES_LONG_MSG_BITS * 2; i += 2) {
	   first	=  m [i];
	   second	=  m [i + 1];
	   delta	= first - second;
	   if (delta < 0)
              delta = -delta;

//         if (delta < 256) {
//            bits [i / 2] = bits [i / 2 - 1];
//         }
//         else
           if (first == second) {
                /* Checking if two adjacent samples have the same magnitude
                 * is an effective way to detect if it's just random noise
                 * that was detected as a valid preamble. */
                bits [i / 2] = 2; /* error */
                if (i < MODES_SHORT_MSG_BITS * 2)
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
uint8_t bits	[MODES_LONG_MSG_BITS];
uint8_t	msg 	[MODES_LONG_MSG_BITS / 8];
uint16_t aux 	[MODES_LONG_MSG_BITS * 2];
uint32_t j, k;
bool	use_correction =  false;
double	correlation;
int	high;
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
	for (j = 0; j < mlen - MODES_FULL_LEN * 2; j++) {
	   int  delta, i, errors;
	   bool good_message = false;
	   if (use_correction)
	      goto good_preamble; /* We already checked it. */

	   correlation = 0;
	   for (k = 0; k < 14; k ++) 
	      correlation += correlationVector [k] * m [j + k];

	   avgValue	= 0.9999 * avgValue + 0.0001 * m[j];
	   avg_corr = 0.99999 * avg_corr + 0.00001 * correlation;
	   if (correlationCounter < 100000) {
	      correlationCounter ++;
	      continue;
	   }

	   if (correlation < 2.5 * avg_corr)
	      continue;

	   if (m [j    ] < 2.5 * avgValue)
	      continue;
	   if (!(m [j]   > m [j+1] &&
	         m [j+1] < m [j+2] &&
	         m [j+2] > m[ j+3] &&
	         m [j+3] < m [j] &&
	         m [j+4] < m [j] &&
	         m [j+5] < m [j] &&
	         m [j+6] < m [j] &&
	         m [j+7] > m [j+8] &&
	         m [j+8] < m [j+9] &&
	         m [j+9] > m [j+6]))
	      continue;

	   high = (m [j] + m [j+2] + m [j+7] + m [j+9]) / 6;
	   if (m [j + 4] >= high || m [j + 5] >= high)
	      continue;
	   if (m [j + 11] >= high ||
	       m [j + 12] >= high ||
               m [j + 13] >= high ||
	       m [j + 14] >= high)
	      continue;

	   stat_valid_preamble++;
	   validPreambles -> display ((int)stat_valid_preamble);

good_preamble:
//	If the previous attempt with this message failed, retry using
//	magnitude correction. 
	   if (use_correction) {
	      memcpy (aux, &m [j + MODES_PREAMBLE_US * 2], sizeof(aux));
              if (j && detectOutOfPhase (& m [j])) {
	         applyPhaseCorrection (& m [j]);
	         stat_out_of_phase++;
	      }
//	Restore the original message if we used magnitude correction. 
	      errors = decodeBits (bits, &m [j + MODES_PREAMBLE_US * 2]);
	      memcpy (&m[j + MODES_PREAMBLE_US * 2], aux, sizeof (aux));
	   }
	   else
	      errors = decodeBits (bits, &m [j + MODES_PREAMBLE_US * 2]);

        /* Pack bits into bytes */
	   for (i = 0; i < MODES_LONG_MSG_BITS / 8; i ++) {
	      msg [i] = 0;
	      for (k = 0; k < 8; k ++)
	         msg [i] |= bits [8 * i + k] << (7 - k);
	   }

	   int msgtype	= msg [0] >> 3;
	   int msglen	= messageLenByType (msgtype) / 8;

//	Last check, high and low bits are different enough in magnitude
//	to mark this as real message and not just noise? 
	   delta = 0;
	   for (i = 0; i < msglen * 8 * 2; i += 2) {
	      delta += abs (m [j + i + MODES_PREAMBLE_US * 2] -
	                    m [j + i + MODES_PREAMBLE_US * 2 + 1]);
	   }
	   delta /= msglen * 4;

//
/*	If we reached this point, and error is zero, we are very likely
 *	with a Mode S message in our hands, but it may still be broken
 *	and CRC may not be correct. This is handled by the next layer.
 */
	   if (errors == 0 || ((handle_errors == STRONG_ERRORFIX) && errors < 3)) {
	      message mm (handle_errors, icao_cache, msg);

	      if (mm. is_crcok () || use_correction) {
	         if (errors == 0)
	            stat_demodulated++;
	         if (mm. errorbit == -1) {
	            if (mm. is_crcok ()) {
	               stat_goodcrc++;
	               goodCrc	-> display ((int)stat_goodcrc);
	            }
	         }
	         else {
	            stat_fixed++;
	            fixed	-> display ((int)stat_fixed);
	            if (mm. errorbit < MODES_LONG_MSG_BITS) {
	               stat_single_bit_fix++;
	               single_bit_fixed -> display ((int)stat_single_bit_fix);
	            }
	            else {
	               stat_two_bits_fix++;
	               two_bit_fixed -> display ((int)stat_two_bits_fix);
	            }
	         }
	      }

//	Skip this message if we are sure it's fine. */
	      if (mm. is_crcok ()) {
	         update_view (&m [j], true);
	         j += (MODES_PREAMBLE_US + (msglen * 8)) * 2;
	         good_message = true;
	         if (use_correction) {
	            stat_phase_corrected ++;
	            phase_corrected -> display ((int)stat_phase_corrected);
	         }
//	Pass data to the next layer */

	         table [msgtype & 0x1F] ++;	
	         update_table (msgtype & 0x1F, table [msgtype & 0x1F]);
	         useModesMessage (&mm);
	      }
	      else
	      if (show_preambles)
	         update_view (&m [j], false);
//
//	and try for the next
	      continue;
	   }

//	Retry with phase correction if possible. */
	   if (!good_message && !use_correction) {
	      j--;
	      use_correction = true;
	   }
	   else {
	      use_correction = false;
	   }
	}
}

/*
 *	When a new message is available, because it was decoded from the
 *	device, file, or received in the TCP input port, or any other
 *	way we can receive a decoded message, we call this function in order
 *	to use the message.
 *
 *	Basically this function passes a raw message to the upper layers for
 *	further processing and visualization.
 */
void	qt1090::useModesMessage (message *mm) {

	if ((!check_crc) || (mm -> is_crcok ())) {
/*
 *	Track aircrafts in interactive mode or if the HTTP
 *	interface is enabled.
 */
	   if (interactive || (stat_http_requests > 0)) {
	      (void) interactiveReceiveData (this, mm);
	   }
/*
 *	In non-interactive way, display messages on standard output.
 */
	   if (!interactive) {
	      mm -> displayMessage (check_crc);
	      printf ("\n");
	   }
	}
}

//
/* ============================= Networking =================================
 * Note: here we disregard any kind of good coding practice in favor of
 * extreme simplicity, that is:
 *
 * 1) We only rely on the kernel buffers for our I/O without any kind of
 *    user space buffering.
 * 2) We don't register any kind of event handler, from time to time a
 *    function gets called and we accept new connections. All the rest is
 *    handled via non-blocking I/O and manually pulling clients to see if
 *    they have something new to share with us when reading is needed.
 */

#define NET_SERVICE_HTTP	0
#define	NUMBER_OF_SERVICES	1
struct {
    const char *descr;
    int *socket;
    int port;
}	modesNetServices [NUMBER_OF_SERVICES] = {
	{"HTTP server",    NULL, MODES_NET_HTTP_PORT},
};

void 	qt1090::serverInit (void) {
int j = 0;
        modesNetServices [0]. socket = &https;
	memset (clients, 0, sizeof (clients));
	maxfd = -1;

	int s = anetTcpServer (aneterr, modesNetServices [j] .port, NULL);
	if (s == -1) {
	   fprintf (stderr,
	            "Error opening the listening port %d (%s): %s\n",
	            modesNetServices [j]. port,
	            modesNetServices [j]. descr,
	            strerror (errno) );
	   exit (1);
	}

	anetNonBlock (aneterr, s);
	*modesNetServices [j]. socket = s;
	signal (SIGPIPE, SIG_IGN);
}

/*
 *	This function gets called when new data is arriving.
 */
void qt1090::AcceptClients (void) {
int fd, port;
int j	=0;
struct client *c;

	while (true) {
	   fd = anetTcpAccept (aneterr, *modesNetServices[j].socket,
	                                                      NULL, &port);
	   if (fd == -1) {
	      if (errno != EAGAIN)
	         printf ("Accept %d: %s\n", *modesNetServices[j].socket,
	                                                  strerror (errno));
	      return;
	   }

	   if (fd >= MODES_NET_MAX_FD) {
	      xclose (fd);
	      return; /* Max number of clients reached. */
	   }

	   anetNonBlock (aneterr, fd);
	   c =  (struct client *)malloc (sizeof (*c));
	   c -> service = *modesNetServices [j].socket;
	   c -> fd = fd;
	   c -> buflen = 0;
	   clients [fd] = c;
	   anetSetSendBuffer (aneterr, fd, MODES_NET_SNDBUF_SIZE);

	   if (maxfd < fd)
	      maxfd = fd;
	}
}

/* On error free the client, collect the structure, adjust maxfd if needed. */
void	qt1090::FreeClient (int fd) {
	xclose (fd);
	free (clients [fd]);
	clients [fd] = NULL;

//	printf ("Closing client %d\n", fd);

//	If this was our maxfd, scan the clients array to find the new max.
//	Note that we are sure there is no active fd greater than the closed
//	fd, so we scan from fd-1 to 0. */
	if (maxfd == fd) {
        int j;

           maxfd = -1;
           for (j = fd - 1; j >= 0; j--) {
	      if (clients [j]) {
	         maxfd = j;
	         break;
	      }
           }
	}
}

/* Turn an hex digit into its 4 bit decimal value.
 * Returns -1 if the digit is not in the 0-F range. */
int hexDigitVal (int c) {
	c = tolower (c);
	if (c >= '0' && c <= '9')
	   return c-'0';
	else
	if (c >= 'a' && c <= 'f')
	   return c-'a'+10;
	else
	   return -1;
}


/* Return a description of planes in json. */
char 	*qt1090::aircraftsToJson (int *len) {
aircraft *plane	= aircrafts;
char buf [512];
int l;
std::string Jsontxt;
	Jsontxt.append ("[\n");
	while (plane != NULL) {
	   if (plane -> lat != 0 && plane -> lon != 0) {
              l = snprintf (buf, 512,
                "{\"hex\":\"%s\", \"flight\":\"%s\", \"lat\":%f, "
                "\"lon\":%f, \"altitude\":%d, \"track\":%d, "
                "\"speed\":%d},\n",
                plane -> hexaddr, plane -> flight, plane -> lat, plane -> lon,
	        plane -> altitude, plane -> track, plane -> speed);
	      Jsontxt. append (buf);
//	Resize if needed. */
	   }
	   plane = plane -> next;
	}
//	Remove the final comma if any, and closes the json array. */
	if (Jsontxt. at (Jsontxt. length () - 2) == ',') {
	   Jsontxt. pop_back ();
	   Jsontxt. pop_back ();
	   Jsontxt. push_back ('\n');
	}
	Jsontxt. append ("]\n");
	char * res = new char [strlen (Jsontxt. c_str ())];
	for (int i = 0; i < strlen (Jsontxt. c_str ()); i ++)
	   res [i] = Jsontxt. c_str () [i];
	*len	= strlen (Jsontxt. c_str ());
	return res;
}

//////////////////////////////////////////////////////////////////////////

//	this slot is called upon the arrival of data
void	qt1090::processData (void) {
int16_t lbuf [MODES_DATA_LEN / 2];
	while (device -> Samples () > MODES_DATA_LEN / 2) {
	   device -> getSamples (lbuf, MODES_DATA_LEN / 2);
	   memcpy (&magnitudeVector [0],
	           &magnitudeVector [MODES_DATA_LEN / 2],
	           ((MODES_FULL_LEN - 1) * sizeof (uint16_t)) * 4 / 2);
	   memcpy (&magnitudeVector [(MODES_FULL_LEN - 1) * 4 / 2],
	           lbuf, MODES_DATA_LEN / 2 * sizeof (int16_t));

	   detectModeS (magnitudeVector, data_len / 2);
	   interactiveRemoveStaleAircrafts (this);
//	Refresh screen when in interactive mode. */
	   if (net) {
	      AcceptClients ();
	      ReadFromClients ();
	   }
	   if (interactive &&
	      (mstime () - interactive_last_update) >
	                             MODES_INTERACTIVE_REFRESH_TIME) {
	      showPlanes (aircrafts, metric);
	      interactive_last_update = mstime();
	   }
	}
}
//
/////////////////////////////////////////////////////////////////////////
//	http handling
/////////////////////////////////////////////////////////////////////////

#define MODES_CONTENT_TYPE_HTML "text/html;charset=utf-8"
#define MODES_CONTENT_TYPE_JSON "application/json;charset=utf-8"

/*
 *	Get an HTTP request header and write the response to the client.
 *	Again here we assume that the socket buffer is enough without doing
 *	any kind of userspace buffering.
 *
 *	Returns 1 on error to signal the caller the client connection should
 *	be closed.
 */
static
int	handleHTTPRequest (void *ctx, struct client *c) {
qt1090 *context = static_cast <qt1090 *> (ctx);
char hdr [512];
int clen, hdrlen;
int httpver, keepalive;
char *p, *url, *content;
char *ctype;

//	printf ("\nHTTP request: %s\n", c -> buf);

/*	Minimally parse the request. */
	httpver = (strstr (c -> buf, "HTTP/1.1") != NULL) ? 11 : 10;
	if (httpver == 10) {
/*	HTTP 1.0 defaults to close, unless otherwise specified. */
	   keepalive = strstr (c -> buf, "Connection: keep-alive") != NULL;
	}
	else
	if (httpver == 11) {
/*	HTTP 1.1 defaults to keep-alive, unless close is specified. */
	   keepalive = strstr (c -> buf, "Connection: close") == NULL;
	}

/*	Identify the URL. */
	p = strchr (c -> buf, ' ');
	if (p == 0)
	   return 1; /* There should be the method and a space... */
	url = ++p; /* Now this should point to the requested URL. */

	p = strchr (p, ' ');
	if (p == NULL)
	   return 1; /* There should be a space before HTTP/... */
	*p = '\0';

/*	Select the content to send, we have just two so far:
 *	"/" -> Our google map application.
 *	"/data.json" -> Our ajax request to update planes.
 */
	if (strstr (url, "/data.json")) {
	   content = (char *)context -> aircraftsToJson (&clen);
	   ctype = (char *)MODES_CONTENT_TYPE_JSON;
	}
	else {
	   struct stat sbuf;
	   FILE	*fd;

	   if (stat ("gmap.html", &sbuf) != -1 &&
	       (fd = fopen ("gmap.html", "r")) != NULL) {
	      content = new char [sbuf. st_size];
	      if (fread (content, 1, sbuf.st_size, fd) < sbuf. st_size) {
	         snprintf (content, sbuf.st_size,
	                      "Error reading from file: %s",
	                                              strerror(errno));
	      }
	      if (fd != NULL)
	         fclose (fd);
	      clen = sbuf.st_size;
	   }
	   else {
	      content	= new char [128];
	      clen = snprintf (content, 128,
	                   "Error opening HTML file: %s", strerror(errno));
	   }
	   ctype = (char *)MODES_CONTENT_TYPE_HTML;
	}
/*
 *	Create the header and send the reply.
 */
	hdrlen = snprintf (hdr, sizeof (hdr),
	                       "HTTP/1.1 200 OK\r\n"
	                       "Server: qt1090\r\n"
	                       "Content-Type: %s\r\n"
	                       "Connection: %s\r\n"
	                       "Content-Length: %d\r\n"
	                       "Access-Control-Allow-Origin: *\r\n"
	                       "\r\n",
	                       ctype,
	                       keepalive ? "keep-alive" : "close",
	                       clen);

/*	Send header and content. */
	if (write (c -> fd, hdr, hdrlen) != hdrlen ||
	    write (c -> fd, content, clen) != clen) {
	   free (content);
	   return 1;
	}

	free (content);
	context -> stat_http_requests++;
	return !keepalive;
}

void	qt1090::ReadFromClients (void) {
int j;
struct client *c;

        for (j = 0; j <= maxfd; j++) {
	   struct client *c = clients [j];
           if ((c != NULL) && (c -> service == https))
              ReadFromClient (c,(char *)"\r\n\r\n", handleHTTPRequest);
        }
}

void	qt1090::ReadFromClient (struct client *c,
	                        char *sep,
	                        int(*handler)(void *, struct client *)) {
	while (true) {
	   int left = CLIENT_BUF_SIZE - c -> buflen;
	   int nread = read (c -> fd, &c -> buf [c -> buflen], left);
//	   int nread = read (c -> fd, c -> buf + c -> buflen, left);
	   int fullmsg = 0;
	   int i;
	   char *p;

	   if (nread <= 0) {
	      if (nread == 0 || errno != EAGAIN) {
                /* Error, or end of file. */
	          FreeClient (c -> fd);
	      }
	      break; /* Serve next client. */
	   }

	   c -> buflen += nread;

//	Always null-term so we are free to use strstr() */
	   c -> buf [c -> buflen] = '\0';

//	If there is a complete message there must be the separator 'sep'
//	in the buffer, note that we full-scan the buffer at every read
//	for simplicity. */
	   while ((p = strstr (c -> buf, sep)) != NULL) {
	      i = p - c -> buf; /* Turn it as an index inside the buffer. */
	      c -> buf[i] = '\0'; /* handler expects null terminated strings. */
//	Call the function to process the message. It returns 1
//	on error to signal we should close the client connection. */
	      if (handler (this, c) != 0) {
	         FreeClient (c -> fd);
	         return;
	      }

//	Move what's left at the start of the buffer. */
	      i += strlen(sep); /* Separator is part of the previous msg. */
	      memmove (c -> buf, c -> buf + i, c -> buflen-i);
	      c -> buflen -= i;
	      c -> buf [c -> buflen] = '\0';
//	Maybe there are more messages inside the buffer.
//	Start looping from the start again. */
	      fullmsg = 1;
	   }
//	If our buffer is full discard it, this is some badly
//	formatted shit. */
	   if (c -> buflen == CLIENT_BUF_SIZE) {
	      c -> buflen = 0;
//	If there is garbage, read more to discard it ASAP. */
	      continue;
	   }
//	If no message was decoded process the next client, otherwise
//	read more data from the same client. */
	   if (!fullmsg)
	      break;
	}
}

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
	index = index & 0x1F;
	QString name = "DF";
	name. append (QString::number (index));
	name. append (":  ");
	name. append (QString::number (newval));
	tableWidget -> setItem (index, 1, new QTableWidgetItem (name));
}

void	qt1090::handle_show_preamblesButton (void) {
	show_preambles	= !show_preambles;
}

