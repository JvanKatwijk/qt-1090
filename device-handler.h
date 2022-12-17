#
/*
 *    Copyright (C) 2010, 2011, 2012
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
 */
#ifndef	__DEVICE_HANDLER__
#define	__DEVICE_HANDLER__
#
/*
 *	We have to create a simple virtual class here, since we
 *	want the interface with different devices (including  filehandling)
 *	to be transparent
 */
#include	"adsb-constants.h"
#include	<complex>
#include	<QThread>
using namespace std;

class	deviceHandler: public QThread {
Q_OBJECT
public:
			deviceHandler 	();
virtual			~deviceHandler 	();
virtual	void		startDevice	();
virtual	void		stopDevice	();
virtual	int		getSamples	(std::complex<float> *, int);
virtual	int		Samples		();
virtual	int		nrBits		();
signals:
	void		dataAvailable	();
};
#endif

