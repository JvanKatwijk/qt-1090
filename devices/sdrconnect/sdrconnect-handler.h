#
/*
 *    Copyright (C) 2026
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the qt-1090
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
 *    along with Qt-1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QSettings>
#include	<QString>
#include	"device-handler.h"
#include	"ringbuffer.h"
#include	"packet-handler.h"
#include	"ui_sdrconnect-widget.h"
//
//	Implements the basic functions of "deviceHandler"
class	QFrame;

class	sdrConnectHandler: public deviceHandler,
	                              Ui_sdrconnectWidget {
Q_OBJECT
public:
		sdrConnectHandler	(QSettings *, int);
		~sdrConnectHandler	() override;
//	void	startDevice		() override;
//	void	stopDevice		() override;
	int32_t	getSamples		(std::complex<float> *b,
	                                        int32_t size) override;
	int32_t	Samples			() override;
	int	nrBits			() override;
private:
	QSettings	*settings;
	int		theFrequency;
	RingBuffer<std::complex<float>> _O_Buffer;
	packetHandler	*theDealer;
	bool		OK_to_run;
	QFrame		*myFrame;
public slots:
	void	handle_hostName		();
	void	connection_failed	();
	void	signalPower		(double v);
	void	thereisData		(int);
	void	rateOK			(int, int);
	void	rateError		();
	void	show_dropCount		(int);
};



