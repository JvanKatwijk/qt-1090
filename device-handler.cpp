#
/*
 *    Copyright (C) 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the qt-1090 program
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
 *
 *      qt-1090 is based on the dump1090 program 
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 */
#include	<stdint.h>
#include	<stdio.h>
#include	"device-handler.h"

	deviceHandler::deviceHandler 	(void) {}
	deviceHandler::~deviceHandler 	(void) {}
void	deviceHandler::startDevice	(void) { fprintf (stderr, "dummy gestart\n");}
void	deviceHandler::stopDevice	(void) {}
int	deviceHandler::getSamples	(int16_t *, int) { return 0; }
int	deviceHandler::Samples		(void) {return 0;}
