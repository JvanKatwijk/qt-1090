
#
/*
 *      qt_1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *
 *	Copyright (C) 2018
 *	Jan van Katwijk (J.vanKatwijk@gmail.com)
 *	Lazy Chair Computing
 *
 *	This file is part of the qt_1090
 *
 *    qt_1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    qt_1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with qt_1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#pragma once
#include        <QObject>
#include        <QString>
#include        <QSettings>
#include        <QTimer>
#include        <QTcpSocket>
#include        <QTcpServer>
#include        <QAbstractSocket>
#include        <QJsonArray>
#include        <QJsonDocument>
#include        <QJsonObject>
#include	<QString>
#include	<atomic>
#include	<string>
#include	<complex>

class	qt1090;

class	httpHandler : public QTcpServer {
Q_OBJECT
public:
	httpHandler	(qt1090 *,
	                 std::complex<float>,
	                 int,
	                 bool autoBrowser);
	~httpHandler	();
bool	isConnected	();
void	closeMap	();
private:
	qt1090		*parent;
        QString		browserAddress;
	int		mapPort;
	bool		autoBrowser;
	std::complex<float>	homeAddress;
	QTimer                  delayTimer;
        bool                    close_map_on_exit;
        QString                 theMap          (std::complex<float> address);
        bool                    connection_stopped;
        std::atomic<bool>       closingInProgress;
	bool			closingRequest;
        int                     maxDelay;

signals:
	void			mapClose_processed	();
private slots:
        void            newConnection           ();
        void            readSocket              ();
        void            discardSocket           ();
        void            handle_timeOut          ();
        void            onSocketError           (QAbstractSocket::SocketError);
};

