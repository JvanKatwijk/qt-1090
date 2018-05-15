#
/*
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

#ifndef	__RESPONDER__
#define	__RESPONDER__
#include        <stdio.h>
#include	<stdio.h>
#include	<qhttpserver.h>
#include	<qhttprequest.h>
#include	<qhttpresponse.h>

class	aircraft;

class Responder : public QObject {
Q_OBJECT

public:
        Responder(QHttpRequest *req, QHttpResponse *resp, aircraft *);
        ~Responder (void);

signals:
        void done (void);

private:
        QScopedPointer<QHttpRequest> m_req;
	QHttpResponse	*m_resp;
	void		sendMap		(QHttpResponse *);
	void		sendPlaneData	(QHttpResponse *, aircraft *);
	aircraft	*planeList;
};
#endif


