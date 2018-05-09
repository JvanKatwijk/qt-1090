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

#ifndef __SDRPLAY_HANDLER__
#define	__SDRPLAY_HANDLER__

#include        <QObject>
#include        <QFrame>
#include        <QSettings>
#include	<dlfcn.h>
#include	<atomic>
#include	"ringbuffer.h"
#include	<pthread.h>
#include	"device-handler.h"
#include        "ui_sdrplay-widget.h"
#include	"mirsdrapi-rsp.h"

typedef void (*mir_sdr_StreamCallback_t)(int16_t	*xi,
	                                 int16_t	*xq,
	                                 uint32_t	firstSampleNum, 
	                                 int32_t	grChanged,
	                                 int32_t	rfChanged,
	                                 int32_t	fsChanged,
	                                 uint32_t	numSamples,
	                                 uint32_t	reset,
	                                 void		*cbContext);
typedef	void	(*mir_sdr_GainChangeCallback_t)(uint32_t	gRdB,
	                                        uint32_t	lnaGRdB,
	                                        void		*cbContext);

///////////////////////////////////////////////////////////////////////////
class	sdrplayHandler: public deviceHandler, public Ui_sdrplayWidget {
Q_OBJECT
public:
		sdrplayHandler          (QSettings *, int);
		~sdrplayHandler		(void);
	void	startDevice		(void);
	void	stopDevice		(void);
	int	getSamples		(int16_t *buffer, int amount);
	int	Samples			(void);
	RingBuffer<int16_t> *dataBuffer;
	void		signalData	(void);
private:
	int16_t         hwVersion;
        uint32_t        numofDevs;
        int16_t         deviceIndex;
        QSettings       *sdrplaySettings;
        QFrame          *myFrame;
        int32_t         freq;
	bool		enable_agc;
private slots:
        void            setExternalGain		(int);
        void            agcControl_toggled      (void);
        void            set_ppmControl          (int);
        void            set_antennaControl      (const QString &);

};
#endif

