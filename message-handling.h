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
 */

#ifndef	__MESSAGE_HANDLING__
#define	__MESSAGE_HANDLING__

#include	"adsb-constants.h"
#include	"crc-handling.h"

class message {
public:
	int msgtype;                /* Downlink format # */
	int errorbit;               /* Bit corrected. -1 if no bit corrected. */
	int heading;
	int mesub;                  /* Extended squitter message subtype. */
	int velocity;               /* Computed from EW and NS velocity. */
private:
	uint8_t msg [LONG_MSG_BITS / 8]; /* Binary message. */
	int msgbits;                /* Number of bits in message */
	uint32_t crc;               /* Message CRC */
	int crcok;                  /* True if CRC was valid */
	int aa1, aa2, aa3;          /* ICAO Address bytes 1 2 and 3 */
	icaoCache	*icao_cache;
/* DF 11 */
	int ca;                     /* Responder capabilities. */

/* DF 17 */
public:
	int metype;                 /* Extended squitter message type. */
	int raw_latitude;           /* Non decoded latitude */
	int raw_longitude;          /* Non decoded longitude */
	int altitude;
	char flight[9];             /* 8 chars flight number. */
	int fflag;                  /* 1 = Odd, 0 = Even CPR message. */
private:
	int heading_is_valid;
	int aircraft_type;
	int tflag;                  /* UTC synchronized? */
	int ew_dir;                 /* 0 = East, 1 = West. */
	int ew_velocity;            /* E/W velocity. */
	int ns_dir;                 /* 0 = North, 1 = South. */
	int ns_velocity;            /* N/S velocity. */
	int vert_rate_source;       /* Vertical rate source. */
	int vert_rate_sign;         /* Vertical rate sign. */
	int vert_rate;              /* Vertical rate. */

/* DF4, DF5, DF20, DF21 */
	int fs;                     /* Flight status for DF4,5,20,21 */
	int dr;                     /* Request extraction of downlink request. */
	int um;                     /* Request extraction of downlink request. */
	int identity;               /* 13 bits identity (Squawk). */

/* Fields used by multiple message types. */
	int	 unit;
//
//	operators
public:
		message		(int, icaoCache *, uint8_t *msg);
		~message	(void);
void		displayMessage	(bool);
uint32_t	getAddr		(void);
bool		is_crcok	(void);
private:

bool    bruteForceAP			(uint8_t *msg);
void	setFlightName			(uint8_t *msg);
void	useModesMessage			(void);
void	recordExtendedSquitter		(uint8_t *msg);
void	print_msgtype_0			(void);
void	print_msgtype_4_20		(void);
void	print_msgtype_5_21		(void);
void	print_msgtype_11		(void);
void	print_msgtype_17		(void);
int	decodeAC12Field			(uint8_t *msg, int *unit);
int	decodeAC13Field			(uint8_t *msg, int *unit);
};

#endif


