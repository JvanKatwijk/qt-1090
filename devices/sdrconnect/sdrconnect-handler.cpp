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
 *    along with qt-1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//
//	handler for connecting to the SDRconnect program.
//	
#include	"sdrconnect-handler.h"
#include	<QFrame>

	sdrConnectHandler::sdrConnectHandler	(QSettings *s, int freq):
	                                             _O_Buffer (16 * 32768) {
	settings	= s;
	theFrequency	= freq;
	OK_to_run	= false;
	theDealer	= nullptr;
	myFrame		= new QFrame (nullptr);
	setupUi (this -> myFrame);
	myFrame -> show ();
	hostnameLabel   -> setInputMask ("000.000.000.000");
        hostnameLabel   -> setText ("127.0.0.1");
	portLabel	-> setValue (5454);
	connect (connectButton, &QPushButton::clicked,
                 this, &sdrConnectHandler::handle_hostName);
}

//	handle hostname is called whenever the user acknowledges the
//	hostname, no guarantee that there is a connection
void	sdrConnectHandler::handle_hostName	() {
QString	hostName	= hostnameLabel -> text ();
int	portNumber	= portLabel	-> value ();
	if (theDealer != nullptr)
	   return;
	theDealer	= new packetHandler (settings,
	                                     hostName, portNumber,
	                                     theFrequency, &_O_Buffer);
	connect (theDealer, &packetHandler::connection_failed,
	         this, &sdrConnectHandler::connection_failed);
	connect (theDealer, &packetHandler::signalPower,
	         this, &sdrConnectHandler::signalPower);
	connect (theDealer, &packetHandler::dataAvailable,
	         this, &sdrConnectHandler::thereisData);
	connect (theDealer, &packetHandler::rateOK,
	         this, &sdrConnectHandler::rateOK);
	connect (theDealer, &packetHandler::rateError,
	         this, &sdrConnectHandler::rateError);
	connect (theDealer, &packetHandler::send_status,
	         this, &sdrConnectHandler::show_dropCount);
	fprintf (stderr, "we hebben een packethandler gelanceerd\n");
}

	sdrConnectHandler::~sdrConnectHandler		() {
	OK_to_run	= false;
	myFrame	-> hide ();
	if (theDealer != nullptr) {
	   delete theDealer;
	}
}

int32_t	sdrConnectHandler::getSamples		(std::complex<float> *b,
	                                                       int32_t size) {
	if (!OK_to_run)
	   return 0;
	return _O_Buffer. getDataFromBuffer (b, size);
}

int32_t	sdrConnectHandler::Samples		() {
	if (!OK_to_run)
	   return 0;
	return _O_Buffer. GetRingBufferReadAvailable ();
}

int	sdrConnectHandler::nrBits		() {
	return 12;
}

void	sdrConnectHandler::connection_failed	() {
	statusLabel	-> setText ("Connection failed");
}

void	sdrConnectHandler::signalPower		(double v) {
	(void)v;
}

void	sdrConnectHandler::thereisData	(int amount) {
	emit dataAvailable ();
}

void	sdrConnectHandler::rateOK		(int r1, int r2) {
	OK_to_run	= true;
	statusLabel	-> setText (QString::number (r1) + "-" + QString::number (r2));
}

void	sdrConnectHandler::rateError		() {
	statusLabel	-> setText ("Alas, this does not work");
}

void	sdrConnectHandler::show_dropCount	(int n) {
	if (n == 0)
	   overflowLabel	-> setStyleSheet ("QLabel {color : green}");
	else
	   overflowLabel	-> setStyleSheet ("QLabel {color : red}");
	overflowLabel	-> setText ("   ");
}

	
