#
/*
 *    Copyright (C) 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair programming
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

#ifndef __FILE_HANDLER__
#define	__FILE_HANDLER__

#include	<QString>
#include	"adsb-constants.h"
#include	"ringbuffer.h"
#include	<pthread.h>
#include	<stdio.h>
#include	"device-handler.h"
#include	<atomic>

///////////////////////////////////////////////////////////////////////////
class	fileHandler: public deviceHandler {
public:
		fileHandler	(QString &, bool);
		~fileHandler	(void);
	void	startDevice	(void);
	void	stopDevice	(void);
	int	getSamples	(int16_t *, int);
	pthread_mutex_t data_mutex;     /* Mutex buffer access. */
	pthread_cond_t  data_cond;      /* Conditional variable associated. */
	RingBuffer<int16_t> *dataBuffer;
	std::atomic<bool>	running;
	FILE	*fd;
private:
	pthread_t	filereader_thread;
};
#endif

