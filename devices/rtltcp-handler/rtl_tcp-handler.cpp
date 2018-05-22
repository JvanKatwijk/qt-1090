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
 *    along with qt-1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    A simple client for rtl_tcp
 */

#include	<QSettings>
#include	<QLabel>
#include	<QMessageBox>
#include	<QHostAddress>
#include	<QTcpSocket>
#include	<QFileDialog>
#include	<QDir>
#include	"rtl_tcp-handler.h"
//


	rtltcpHandler::rtltcpHandler	(QSettings *s, int32_t frequency) {
	remoteSettings		= s;
	this	-> vfoFrequency	= frequency;
	theFrame		= new QFrame;
	setupUi (theFrame);
	this	-> theFrame	-> show ();

    //	setting the defaults and constants
	theRate		= 2048000;
	remoteSettings	-> beginGroup ("rtl_tcp_client");
	theGain		= remoteSettings ->
	                          value ("rtl_tcp_client-gain", 20). toInt ();
	thePpm		= remoteSettings ->
	                          value ("rtl_tcp_client-ppm", 0). toInt ();
	basePort = remoteSettings -> value ("rtl_tcp_port", 1234).toInt();
	remoteSettings	-> endGroup ();
	tcp_gain	-> setValue (theGain);
	tcp_ppm		-> setValue (thePpm);
	_I_Buffer	= new RingBuffer<uint8_t>(64 * 32768);
	connected	= false;
	hostLineEdit 	= new QLineEdit (NULL);

	connect (tcp_connect, SIGNAL (clicked (void)),
	         this, SLOT (wantConnect (void)));
	connect (tcp_disconnect, SIGNAL (clicked (void)),
	         this, SLOT (setDisconnect (void)));
	connect (tcp_gain, SIGNAL (valueChanged (int)),
	         this, SLOT (sendGain (int)));
	connect (tcp_ppm, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ppmCorrection (int)));
	state	-> setText ("waiting to start");
}

	rtltcpHandler::~rtltcpHandler	(void) {
	remoteSettings ->  beginGroup ("rtl_tcp_client");
	if (connected) {		// close previous connection
	   stopDevice	();
//	   streamer. close ();
	   remoteSettings -> setValue ("remote-server",
	                               toServer. peerAddress (). toString ());
	   QByteArray datagram;
	}
	remoteSettings -> setValue ("rtl_tcp_client-gain",   theGain);
	remoteSettings -> setValue ("rtl_tcp_client-ppm",    thePpm);
	remoteSettings -> endGroup ();
	toServer. close ();
	delete	_I_Buffer;
	delete	hostLineEdit;
	delete	theFrame;
}
//
void	rtltcpHandler::wantConnect (void) {
QString ipAddress;
int16_t	i;
QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();

	if (connected)
	   return;
	// use the first non-localhost IPv4 address
	for (i = 0; i < ipAddressesList.size(); ++i) {
	   if (ipAddressesList.at (i) != QHostAddress::LocalHost &&
	      ipAddressesList. at (i). toIPv4Address ()) {
	      ipAddress = ipAddressesList. at(i). toString();
	      break;
	   }
	}
	// if we did not find one, use IPv4 localhost
	if (ipAddress. isEmpty())
	   ipAddress = QHostAddress (QHostAddress::LocalHost).toString ();
	remoteSettings -> beginGroup ("rtl_tcp_client");
	ipAddress = remoteSettings ->
	                value ("remote-server", ipAddress). toString ();
	remoteSettings -> endGroup ();
	hostLineEdit -> setText (ipAddress);

	hostLineEdit	-> setInputMask ("000.000.000.000");
//	Setting default IP address
	hostLineEdit	-> show ();
	state	-> setText ("Enter IP address, \nthen press return");
	connect (hostLineEdit, SIGNAL (returnPressed (void)),
	         this, SLOT (setConnection (void)));
}

//	if/when a return is pressed in the line edit,
//	a signal appears and we are able to collect the
//	inserted text. The format is the IP-V4 format.
//	Using this text, we try to connect,
void	rtltcpHandler::setConnection (void) {
QString s	= hostLineEdit -> text ();
QHostAddress theAddress	= QHostAddress (s);

	serverAddress	= QHostAddress (s);
	disconnect (hostLineEdit, SIGNAL (returnPressed (void)),
	            this, SLOT (setConnection (void)));
	toServer. connectToHost (serverAddress, basePort);
	if (!toServer. waitForConnected (2000)) {
	   QMessageBox::warning (theFrame, tr ("sdr"),
	                                   tr ("connection failed\n"));
	   return;
	}

	sendGain (theGain);
	sendRate (theRate);
	sendVFO	(vfoFrequency);
	toServer. waitForBytesWritten ();
	state -> setText ("Connected");
	connected	= true;
}

void	rtltcpHandler::startDevice	(void) {
	if (!connected)
	   return;
	connect (&toServer, SIGNAL (readyRead (void)),
	         this, SLOT (readData (void)));
}

void	rtltcpHandler::stopDevice	(void) {
	if (!connected)
	   return;
	disconnect (&toServer, SIGNAL (readyRead (void)),
	            this, SLOT (readData (void)));
}
//
//	The brave old getSamples. For the dab stick, we get
//	size: still in I/Q pairs,
int32_t	rtltcpHandler::getSamples (int16_t *V, int32_t size) { 
int32_t	amount, i;
uint8_t	tempBuffer [2 * size];
//
	amount = _I_Buffer	-> getDataFromBuffer (tempBuffer, 2 * size);

	for (i = 0; i < amount / 2; i ++) {
	   int16_t re = (int16_t)((int8_t)(tempBuffer [2 * i] - 128));
	   int16_t im = (int16_t)((int8_t)(tempBuffer [2 * i + 1] - 128));
	   V [i] = (re < 0 ? -re : re) + (im < 0 ? -im : im);
	}
	return amount / 2;
}

int32_t	rtltcpHandler::Samples	(void) {
	return  _I_Buffer	-> GetRingBufferReadAvailable () / 2;
}

//	These functions are typical for network use
void	rtltcpHandler::readData	(void) {
uint8_t	buffer [8192];
	while (toServer. bytesAvailable () > 8192) {
	   toServer. read ((char *)buffer, 8192);
	   _I_Buffer -> putDataIntoBuffer (buffer, 8192);
	   if (_I_Buffer -> GetRingBufferReadAvailable () > 256000)
	      emit dataAvailable ();
	}
}
//
//
//	commands are packed in 5 bytes, one "command byte" 
//	and an integer parameter
struct command {
	unsigned char cmd;
	unsigned int param;
}__attribute__((packed));

#define	ONE_BYTE	8

void	rtltcpHandler::sendCommand (uint8_t cmd, int32_t param) {
QByteArray datagram;

	datagram. resize (5);
	datagram [0] = cmd;		// command to set rate
	datagram [4] = param & 0xFF;  //lsb last
	datagram [3] = (param >> ONE_BYTE) & 0xFF;
	datagram [2] = (param >> (2 * ONE_BYTE)) & 0xFF;
	datagram [1] = (param >> (3 * ONE_BYTE)) & 0xFF;
	toServer. write (datagram. data (), datagram. size ());
}

void	rtltcpHandler::sendVFO (int32_t frequency) {
	sendCommand (0x01, frequency);
}

void	rtltcpHandler::sendRate (int32_t theRate) {
	sendCommand (0x02, theRate);
}

void	rtltcpHandler::setGainMode (int32_t gainMode) {
	sendCommand (0x03, gainMode);
}

void	rtltcpHandler::sendGain (int gain) {
	sendCommand (0x04, 10 * gain);
	theGain		= gain;
}

//	correction is in ppm
void	rtltcpHandler::set_ppmCorrection	(int32_t ppm) {
	sendCommand (0x05, ppm);
	thePpm		= ppm;
}

void	rtltcpHandler::setDisconnect (void) {
	if (connected) {		// close previous connection
	   stopDevice	();
	   remoteSettings -> beginGroup ("rtl_tcp_client");
	   remoteSettings -> setValue ("remote-server",
	                               toServer. peerAddress (). toString ());
	   remoteSettings -> setValue ("rtl_tcp_client-gain", theGain);
	   remoteSettings -> setValue ("rtl_tcp_client-ppm", thePpm);
	   remoteSettings -> endGroup ();
	   toServer. close ();
	}
	connected	= false;
	connectedLabel	-> setText (" ");
	state		-> setText ("disconnected");
}

