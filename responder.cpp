#
/*
 *      qt-1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *	
 *	The httpserver software
 *	Copyright 2011-2014 Nikhil Marathe <nsm.nikhil@gmail.com>
 *
 *	qt-1090 Copyright (C) 2018
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
 *    along with qt1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include	"responder.h"
#include	"aircraft-handler.h"

	Responder::Responder (QHttpRequest *request,
	                      QHttpResponse *response,
	                      aircraft	*planeList): m_req (request),
	                                                m_resp (response) {

//	connect (request, SIGNAL (end (void)), response, SLOT (end (void)));
//	Once the request is complete, the response is sent.
//	When the response ends, it deletes itself
//	the Responder object connects to done ()
//	which will lead to it being deleted
//	and this will delete the request.
//	So all 3 are properly deleted.
	this	-> planeList	= planeList;

	connect (response, SIGNAL (done (void)),
	                  this, SLOT (deleteLater (void)));
	
	if (request -> path () == "/data.json")
	   sendPlaneData (response, planeList);
	else
	if (request -> path () == "/")
	   sendMap (response);
}

void	Responder::sendMap (QHttpResponse *response) {
struct stat sbuf;
int	fd = -1;
char	*body;

	if (stat ("gmap.html", &sbuf) != -1 &&
	   (fd = open ("gmap.html", O_RDONLY)) != -1) {
	   body = new char [sbuf. st_size];
	   if (read (fd, body, sbuf.st_size) == -1) {
	      (void)snprintf (body, sbuf.st_size,
                              "Error reading from file: %s",
                                                      strerror(errno));
	   }
	}
	else {
	   body = new char [512];
	   (void) snprintf (body, 512,
                           "Error opening HTML file: %s", strerror(errno));
	}

	response -> setHeader ("Content-Type", "text/html");
	response -> writeHead (200);
	response -> end (body);
	delete[] body;
}

void	Responder::sendPlaneData (QHttpResponse *response,
	                          aircraft *planeList) {
QString	body;

	body	= QString::fromStdString (aircraftsToJson (planeList));	
	response -> setHeader ("Content-Type",
	                       "application/json;charset=utf-8");
	response -> writeHead (200);
	response -> end (body. toUtf8 ());
}

Responder::~Responder (void) {
}

