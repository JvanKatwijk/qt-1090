#
/*
 *    Copyright (C) 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the dump1090 program
 *
 *    dump1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dump1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dump1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	"file-handler.h"
#include	<unistd.h>

static int16_t localBuf [2048];
int	bufP	= 0;

	fileHandler::fileHandler	(QString &fileName,
	                                 bool interactive) {	
const char * c_fileName = fileName. toLatin1 (). data ();
	   if (c_fileName [0] == '-' && c_fileName [1] == '\0') 
	      fd = stdin;
           else {
	      fd = fopen (c_fileName, "r");
	      if (fd == NULL)
	         throw (22);
           }

	   dataBuffer	= new RingBuffer<int16_t> (16 * 32768);
	   interactive	= interactive;
	   running. store (false);
	   pthread_mutex_init (&data_mutex, NULL);
	   pthread_cond_init  (&data_cond,  NULL);
}

	fileHandler::~fileHandler	(void) {
	   fclose (fd);
}

static
void	*filereaderEntry (void *arg) {
fileHandler *f = static_cast<fileHandler *> (arg);
int toread = DATA_LEN;
uint8_t lbuf [DATA_LEN];

	f -> running. store (true);
	while (f -> running. load ()) {
	   int i;
	   size_t nread = fread (lbuf, 1, toread, f -> fd);
	   if (nread <= 0) {
	      return NULL; /* Signal the system. */
	   }

	   for (i = 0; i < nread / 2; i ++) {
	      int16_t re = (int16_t)((int8_t)(lbuf [2 * i] - 128));
	      int16_t im = (int16_t)((int8_t)(lbuf [2 * i + 1] - 128));
	      localBuf [bufP ++] = (re < 0 ? -re : re) +
	                               (im < 0 ? -im : im);
	      if (bufP >= 2048) {
	         f -> dataBuffer -> putDataIntoBuffer (localBuf, 2048);
	         bufP = 0;
	      }
	   }
	   pthread_mutex_lock (&f -> data_mutex);
	   if (f -> dataBuffer -> GetRingBufferReadAvailable () > 200000) {
	      pthread_cond_signal  (&f -> data_cond);
	      pthread_mutex_unlock (&f -> data_mutex);
	      usleep (100000);
	   }
	}
	return NULL;
}

void	fileHandler::startDevice	(void) {
	running. store (true);
	pthread_create (&filereader_thread,
	                NULL,
	                filereaderEntry,
	                (void *)(this));
}

void	fileHandler::stopDevice (void) {
	running. store (false);
}

int	fileHandler::getSamples	(int16_t *buffer, int amount) {
	while (dataBuffer -> GetRingBufferReadAvailable () < amount)
	   pthread_cond_wait (&data_cond, &data_mutex);

	dataBuffer      -> getDataFromBuffer (buffer, amount);
	return amount;
}


