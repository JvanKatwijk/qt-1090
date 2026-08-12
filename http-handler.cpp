#
/*
 *      The http handling in SDRunoPlugin_1090 is
 *	based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *
 *	Copyright (C) 2018
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Computing
 *
 *	This file is part of the SDRunoPlugin_1090
 *
 *    SDRunoPlugin_1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    SDRunoPlugin_1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with SDRunoPlugin_1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include	<QDesktopServices>
#include	<QFile>
#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<sys/types.h> 
#include	"http-handler.h"
#include	"qt-1090.h"
#include	"aircraft-handler.h"


	httpHandler::httpHandler (qt1090 *parent,
	                          std::complex<float> homeAddress,
	                          int	mapPort,
	                          bool autoBrowser) {
	                          
	this	-> parent	= parent;
	this	-> homeAddress	= homeAddress;
	this	-> mapPort	= mapPort;
	this	-> autoBrowser	= autoBrowser;

	QString	browserAddress	= QString ("http://localhost") + ":" +
	                                           QString::number (mapPort);
	delayTimer. setSingleShot (true);

	if (!QDesktopServices::openUrl(QUrl (browserAddress))) {
	   throw (21);
	}

	if (!this -> listen (QHostAddress::Any, mapPort)) { 
           throw (22);
        }
                                  
        connect (this, &QTcpServer::newConnection,
                 this, &httpHandler::newConnection);
	connect (&delayTimer, &QTimer::timeout,
	         this, &httpHandler::handle_timeOut);
        connection_stopped      = false;
        closingInProgress. store (false);
	closingRequest		= false;
}

	httpHandler::~httpHandler	() {
	if (this -> isListening ())
           this -> close ();
}

void	httpHandler::closeMap		() {
	closingRequest	= true;
}

void	httpHandler::newConnection	() {
	if (this -> hasPendingConnections ()) {
	   QTcpSocket *socket = this -> nextPendingConnection ();
	   connect (socket, &QTcpSocket::readyRead,
	            this, &httpHandler::readSocket);
	   connect (socket, &QAbstractSocket::disconnected,
	            this, &httpHandler::discardSocket);
	   connect  (socket, &QTcpSocket::errorOccurred,
	             this, &httpHandler::onSocketError);
	}
}
//
void	httpHandler::discardSocket () {
QTcpSocket *socket = reinterpret_cast<QTcpSocket*>(sender());
	socket -> deleteLater ();
}
//
//	If "closingInProgress" is true, the user has indicated that the
//	http driver is to be stopped, with as secondary effect that the
//	browser stops displaying the map and issues an error 1
//	Otherwise, the user killed the browser and the
//	http Handler is killed if the close_map_on_exit is true
//	Note that the function is a "slot", issuing a signal
//	that directly kills the httpHandler will crash the system
void	httpHandler::onSocketError (QAbstractSocket::SocketError socketerror) {
//QTcpSocket *socket = reinterpret_cast<QTcpSocket*>(sender());
	if (socketerror == QAbstractSocket::RemoteHostClosedError) {
	   if (closingInProgress. load ()) {	// reacting on button switch
	      connect (this, &httpHandler::mapClose_processed,
	               parent, &qt1090::http_terminate);
	      fprintf (stderr, "Going to close a map\n");
	      emit mapClose_processed ( );
	   }
	   else {	// 
//	      fprintf (stderr, "de http handler zou moeten sluiten\n");
	      if (close_map_on_exit)
	         if (!delayTimer. isActive ())
	            delayTimer. start (1000);
	   }
	}
}
//
bool	httpHandler::isConnected () {
	return !connection_stopped;
}

void	httpHandler::readSocket () {
QTcpSocket *worker = qobject_cast<QTcpSocket *> (sender ());
	if (worker == nullptr)
	   return;
	delayTimer. stop ();	// it seems we are reconnected
	if (closingInProgress. load ())
	   return;
	closingInProgress. store (false);
	QByteArray data		= worker -> readAll ();
	QString request		= QString (data);
	int version		= request. contains ("HTTP/1.1") ? 11 : 10;
	bool keepAlive	= (version == 11) ? 
	                     !request. contains ("Connection: close"):
	                     request. contains ("Connection: keep-alive");
//	Identify the URL
	QStringList list	= request. split (" ");
	if (list. size () < 2) 
	   return;
	QString askingFor	= list [1];
//	QString content;
	QByteArray theContents;
	QString ctype;
	if (askingFor == "/data.json") {
	   if (closingRequest) {
	      QString xx ="[{\"hex\":\"-100\"}]";
              theContents = xx. toUtf8 ();
           }
	   else {
	      QString xx	= aircraftsToJson (parent -> planeList);
	      theContents	= xx. toUtf8 ();
	   }
	   ctype	= "application/json;charset=utf-8";
	}
        else {
           QString map	= theMap (homeAddress);
           ctype	= "text/html;charset=utf-8";
	   theContents	= map. toUtf8 ();
        }
//	Create the header
	char hdr [2048];
	sprintf (hdr,
	      "HTTP/1.1 200 OK\r\n"
	      "Server: qt-dab\r\n"
	      "Content-Type: %s\r\n"
	      "Connection: %s\r\n"
	      "Content-Length: %d\r\n"
	      "\r\n",
	      ctype. toLatin1 (). data (),
	      keepAlive ? "keep-alive" : "close",
	      (int)(theContents. size ()));
	QByteArray theData = hdr;
	(void)worker -> write (theData);
	(void)worker -> write (theContents);

//	In case we have sent a close signal
	if (closingInProgress. load ()) {
	   worker -> waitForBytesWritten ();
	}
	delayTimer. start (4000);
}
//
//	This timeout is initiated after a (potential) disconnect
//	:Under normal circumstances, after pushing the httpButton,
//	an error is signalled and the error function takes
//	care 
void	httpHandler::handle_timeOut () {
	if (!close_map_on_exit)
	   return;
	fprintf (stderr, "timeout gezien\n");
	connection_stopped	= true;
	if (closingInProgress. load ())
	   connect (this, &httpHandler::mapClose_processed,
	            parent, &qt1090::http_terminate);
	else
	   connect (this, &httpHandler::mapClose_processed,
	            parent, &qt1090::cleanUp_mapHandler);
	emit mapClose_processed ();
}

QString	httpHandler::theMap (std::complex<float> homeAddress) {
int	bodySize;
char	*body;
std::string latitude	= std::to_string (real (homeAddress));
std::string longitude	= std::to_string (imag (homeAddress));
int	index		= 0;
int	cc;
int	teller		= 0;
int	params		= 0;

// read map file from resource file
	QFile file ("res/qt-map.html");
	if (file. open (QFile::ReadOnly)) {
	   QByteArray record_data (1, 0);
	   QDataStream in (&file);
	   bodySize	= file. size ();
	   body		=  (char *)malloc (bodySize + 40);
	   while (!in. atEnd ()) {
	      in . readRawData (record_data. data (), 1);	
	      cc = (*record_data. constData ());
	      if (cc == '$') {
	         if (params == 0) {
	            for (int i = 0; latitude. c_str () [i] != 0; i ++)
	               if (latitude. c_str () [i] == ',')
	                  body [teller ++] = '.';
	               else
	                  body [teller ++] = latitude. c_str () [i];
	            params ++;
	         }
	         else
	         if (params == 1) {
	            for (int i = 0; longitude. c_str () [i] != 0; i ++)
	               if (longitude. c_str () [i] == ',')
	                  body [teller ++] = '.';
	               else
	                  body [teller ++] = longitude. c_str () [i];
	            params ++;
	         }
	         else
	            body [teller ++] = (char)cc;
	      }
	      else
	         body [teller ++] = (char)cc;
	   }
	   body [teller ++] = 0;
	}
	else {
	   fprintf (stderr, "cannot open file\n");
	   return "";
	}

	body [teller ++] = 0;
	QString	res	= QString (body);
//	fprintf (stderr, "The map :\n%s\n", res. c_str ());
	file. close ();
	free (body);
	return res;
}

