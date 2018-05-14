#
/*
 *
 *      qt-1090 is based on  and contains source code from the
 *	dump1090 program 
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *
 *	qt-1090 Copyright (C) 2018
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Programming
 *
 *	This file is part of the qt-1090 program
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
 *    along with qt-1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *

 */

#ifndef	__ADSB_CONSTANTS__
#define	__ADSB_CONSTANTS__

#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<pthread.h>
#include	<stdint.h>
#include	<errno.h>
#include	<unistd.h>
#include	<math.h>
#include	<sys/time.h>
#include	<signal.h>
#include	<fcntl.h>
#include	<ctype.h>
#include	<sys/stat.h>
#include	<sys/ioctl.h>
#include	<sys/select.h>

class	aircraft;
class	icaoCache;

#define MODES_DATA_LEN             (16*16384)   /* 256k */

#define MODES_PREAMBLE_US 8       /* microseconds */
#define MODES_LONG_MSG_BITS 112
#define MODES_SHORT_MSG_BITS 56
#define MODES_FULL_LEN (MODES_PREAMBLE_US + MODES_LONG_MSG_BITS)

#define MODES_UNIT_FEET 0
#define MODES_UNIT_METERS 1

#define MODES_INTERACTIVE_REFRESH_TIME 250      /* Milliseconds */
#define MODES_INTERACTIVE_ROWS 15               /* Rows on screen */
#define MODES_INTERACTIVE_TTL 60                /* TTL before being removed */

#define MODES_NOTUSED(V) ((void) V)

#define	CURRENT_VERSION	"0.6"

#define	NO_ERRORFIX	0
#define	NORMAL_ERRORFIX	1
#define	STRONG_ERRORFIX	2

static inline
long long mstime (void) {
struct timeval tv;
long long mst;

        gettimeofday (&tv, NULL);
        mst = ((long long)tv.tv_sec) * 1000;
        mst += tv.tv_usec/1000;
        return mst;
}

static inline
int	messageLenByType (int type) {
        if (type == 16 || type == 17 ||
            type == 19 || type == 20 || type == 21)
           return MODES_LONG_MSG_BITS;
        else
           return MODES_SHORT_MSG_BITS;
}

#endif

