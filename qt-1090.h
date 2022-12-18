#
/*
 *      qt-1090 is based on and contains source code from dump1090
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.
 *
 *	Copyright (C) 2018
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
 *    along with qt-1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef	__QT_1090__
#define	__QT_1090__

#include	"adsb-constants.h"
#include	<QMainWindow>
#include	<QObject>
#include	<QSettings>
#include	<QMessageBox>
#include	<QCloseEvent>
#include	<QTimer>
#include	<stdio.h>
#include	"http-handler.h"
#include	"message-handling.h"
#include	"ui_qt-1090.h"
#include	"spectrumviewer.h"
#include	<complex>

class	deviceHandler;


class	qt1090: public QMainWindow, private Ui_mainwindow {
Q_OBJECT
public:
	qt1090		(QSettings *, int freq, bool network);
	~qt1090		(void);
private:
	void		finalize	(void);
	void		closeEvent	(QCloseEvent *event);
	int		decodeBits	(uint8_t *bits, uint16_t *m);
	void		detectModeS	(uint16_t *m, uint32_t mlen);
	void		update_table	(int16_t, int);
	int		table [32];
	httpHandler	*httpServer;
	QTimer		screenTimer;
public slots:
	void		processData	();
private:
	spectrumViewer	*viewer;
	pthread_t	reader_thread;
	std::atomic<bool>	running;;
	int		frequency;
	bool		network;
	deviceHandler	*theDevice;
	QSettings	*qt1090Settings;
	uint16_t	*magnitudeVector;
	uint32_t	data_len;	/* Buffer length. */
	int		httpPort;
//	Configuration */
	bool		net;		/* Enable networking. */
	bool		interactive;	/* Interactive mode */
	bool		metric;		/* Use metric units. */
	int		bitstoShow;
	FILE		*dumpfilePointer;
public:
	icaoCache	*icao_cache;
	bool		check_crc;	/* Only display messages with good CRC. */
	int		handle_errors;	/* Level of error correction	*/
	int		interactive_ttl;   /* Interactive mode: TTL before deletion. */
	int		debug;		/* Debugging mode. */

//	Interactive mode */
	aircraft	*planeList;
	long long interactive_last_update;  /* Last screen update in milliseconds */

//	Statistics */
public:
	long long stat_valid_preamble;
	long long stat_goodcrc;
	long long stat_badcrc;
	long long stat_fixed;
	long long stat_single_bit_fix;
	long long stat_two_bits_fix;
	long long stat_http_requests;
	long long stat_phase_corrected;
private slots:
	void	handle_interactiveButton ();
	void	handle_errorhandlingCombo	(const QString &);
	void	handle_httpButton	();
	void	set_ttl			(int);
	void	handle_metricButton	();
	void	updateScreen		();
	void	handle_dumpButton	();
	void	setDevice		(const QString &);
	void	handle_set_coordinatesButton	();
	void	handle_autoBrowser	(int);
};

#endif

