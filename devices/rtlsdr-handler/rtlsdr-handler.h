#
/*
 *    Copyright (C) 2012 .. 2017
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

#ifndef __RTLSDR_HANDLER__
#define	__RTLSDR_HANDLER__

#include	<QSettings>
#include	<QFrame>
#include        <dlfcn.h>
#include	"device-handler.h"
#include	"ringbuffer.h"
#include	"rtl-sdr.h"
#include        "ui_rtlsdr-widget.h"

class	dll_driver;

class	rtlsdrHandler: public deviceHandler, public  Ui_dabstickWidget {
Q_OBJECT
public:
			rtlsdrHandler	(QSettings *, int);
			~rtlsdrHandler	(void);
	void		startDevice	(void);
	void		stopDevice	(void);
	int		getSamples	(int16_t *buffer, int);
	int		Samples		(void);
	RingBuffer<int16_t>	*_I_Buffer;
        struct rtlsdr_dev	*theDevice;
	void		signalData	(void);
private:
	dll_driver	*workerHandle;
	QSettings	*rtlsdrSettings;
	QFrame		*myFrame;
	int		deviceIndex;
	int		*gains;
	int		freq;
	bool		enable_agc;
        int16_t         gainsCount;

private slots:
	void            set_ExternalGain        (const QString &);
        void            set_autogain            (const QString &);
        void            set_ppmCorrection       (int);
};

#endif


