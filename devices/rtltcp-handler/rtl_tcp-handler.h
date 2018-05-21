#
/*
 *    Copyright (C) 2013 .. 2017
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the qt-1090 program
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
 *    along with 1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__RTL_TCP_HANDLER__
#define	__RTL_TCP_HANDLER__

#include	<QtNetwork>
#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QLineEdit>
#include	<QHostAddress>
#include	<QByteArray>
#include	<QTcpSocket>
#include	<QTimer>
#include	<QComboBox>
#include	<stdio.h>
#include	"adsb-constants.h"
#include	"device-handler.h"
#include	"ringbuffer.h"
#include	"ui_rtl_tcp-widget.h"

class	rtltcpHandler: public deviceHandler, Ui_rtl_tcp_widget {
Q_OBJECT
public:
			rtltcpHandler	(QSettings *, int32_t);
			~rtltcpHandler	(void);
	void		startDevice	(void);
	void		stopDevice	(void);
	int32_t		getSamples	(int16_t *V, int32_t size);
	int32_t		Samples		(void);
private slots:
	void		sendGain	(int);
	void		set_ppmCorrection	(int);
	void		readData	(void);
	void		setConnection	(void);
	void		wantConnect	(void);
	void		setDisconnect	(void);
private:
	void		sendVFO		(int32_t);
	void		sendRate	(int32_t);
	void		setGainMode (int32_t gainMode);
	void		sendCommand	(uint8_t, int32_t);
	QLineEdit	*hostLineEdit;
	bool		isvalidRate	(int32_t);
	QSettings	*remoteSettings;
	QFrame		*theFrame;
	int32_t		theRate;
	int32_t		vfoFrequency;
	RingBuffer<uint8_t>	*_I_Buffer;
	bool		connected;
	int16_t		theGain;
	int16_t		thePpm;
	QHostAddress	serverAddress;
	QTcpSocket	toServer;
	qint64		basePort;
};

#endif

