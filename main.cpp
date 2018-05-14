#
/*
 *    Copyright (C) 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of qt-1090
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
 *
 *      qt-1090 is based on the dump1090 program 
 *      Copyright (C) 2012 by Salvatore Sanfilippo <antirez@gmail.com>
 *      all rights acknowledged.

 */
#include        <QApplication>
#include        <QSettings>
#include	<QTranslator>
#include        <QDir>
#include	<QDebug>
#include        <unistd.h>
#include        "adsb-constants.h"
#include	"qt-1090.h"

#ifdef	__HAVE_RTLSDR__
#include	"rtlsdr-handler.h"
#endif
#ifdef	__HAVE_SDRPLAY__
#include	"sdrplay-handler.h"
#endif
#include	"file-handler.h"
#define DEFAULT_INI     ".qt-1090.ini"

#ifndef	GITHASH
#define	GITHASH	"      "
#endif

QString fullPathfor (QString v, QString aa) {
QString fileName;

	if (v == QString (""))
	   return QString ("/tmp/xxx");

	if (v. at (0) == QChar ('/'))           // full path specified
	   return v;

	fileName = QDir::homePath ();
	fileName. append ("/");
	fileName. append (v);
	fileName = QDir::toNativeSeparators (fileName);

	if (!fileName. endsWith (aa))
	   fileName. append (aa);

	return fileName;
}

void    	setTranslator	(QString Language);
deviceHandler   *setDevice (QSettings *dabSettings,
                            int deviceIndex, int freq, char *fileName);
void		showHelp (void);


int     main (int argc, char **argv) {
QString initFileName	= fullPathfor (QString (DEFAULT_INI), ".ini");
qt1090  *MyRadioInterface;
deviceHandler	*theDevice;
QSettings       *dumpSettings;           // ini file
int     j;
char	*fileName	= NULL;
int	deviceIndex	= 0;
int	freq		= 1090000000;

	QCoreApplication::setOrganizationName	("Lazy Chair Computing");
	QCoreApplication::setOrganizationDomain ("Lazy Chair Computing");
	QCoreApplication::setApplicationName	("qt-1090");
	QCoreApplication::setApplicationVersion (QString (CURRENT_VERSION) + " Git: " + GITHASH);

//	Parse the command line options */
	for (j = 1; j < argc; j++) {
	   bool more = j + 1 < argc; /* There are more arguments. */

	   if (!strcmp (argv [j], "--device-index") && more) {
	      deviceIndex	= atoi (argv [++j]);
	   } else
	   if (!strcmp (argv [j],"--freq") && more) {
	       freq = strtoll (argv[++j], NULL, 10);
	   } else
	   if (!strcmp (argv [j],"--ifile") && more) {
	       fileName = strdup (argv[++j]);
	   } else if (!strcmp (argv[j],"--help")) {
	      showHelp ();
	      exit (0);
	   } else {
	      fprintf (stderr,
	            "Unknown or insufficient number of arguments for option '%s'.\n\n",
	                           argv[j]);
	      showHelp ();
              exit (1);
	   }
	}
	
	dumpSettings =  new QSettings (initFileName, QSettings::IniFormat);
/*
 *      Before we connect control to the gui, we have to
 *      instantiate
 */
	QApplication a (argc, argv);
//	setting the language
	QString locale = QLocale::system (). name ();
	qDebug() << "main:" <<  "Detected system language" << locale;
	setTranslator (locale);

	theDevice	= setDevice (dumpSettings, deviceIndex, freq, fileName);
	if (theDevice == NULL) {
	   fprintf (stderr, "sorry, no device found, fatal\n");
	   exit (1);
	}
//	a. setWindowIcon (QIcon (":/dab-radio.ico"));

	MyRadioInterface = new qt1090 (dumpSettings, theDevice);
	MyRadioInterface -> show ();

#if QT_VERSION >= 0x050600
	QGuiApplication::setAttribute (Qt::AA_EnableHighDpiScaling);
#endif
        a. exec ();
/*
 *      done:
 */
	dumpSettings -> sync ();
	delete theDevice;
	fprintf (stderr, "back in main program\n");
	fflush (stdout);
	fflush (stderr);
	qDebug ("It is done\n");
//	delete MyRadioInterface;
	delete dumpSettings;
}

void	setTranslator (QString Language) {
QTranslator *Translator = new QTranslator;

//	German is special (as always)
	if ((Language == "de_AT") || (Language ==  "de_CH"))
	   Language = "de_DE";
//
//	what about Dutch?
	bool TranslatorLoaded =
	             Translator -> load (QString(":/i18n/") + Language);
	qDebug() << "main:" <<  "Set language" << Language;
	QCoreApplication::installTranslator (Translator);

	if (!TranslatorLoaded) {
	   qDebug() << "main:" <<  "Error while loading language specifics" << Language << "use English \"en_GB\" instead";
	   Language = "en_GB";
	}

	QLocale curLocale (QLocale ((const QString&)Language));
	QLocale::setDefault (curLocale);
}

deviceHandler	*setDevice (QSettings *dumpSettings,
	                    int deviceIndex, int freq, char *fileName) {
deviceHandler	*inputDevice	= NULL;
///	OK, everything quiet, now let us see what to do
	if (fileName != NULL) {
	   try {
	      inputDevice = new fileHandler (fileName, true);
	      return inputDevice;
	   } catch (int e) {
	      fprintf (stderr, "Failing to open %s, fatal\n", fileName);	
	      exit (1);
	   }
	}
#ifdef	__HAVE_SDRPLAY__
	try {
	   inputDevice	= new sdrplayHandler (dumpSettings, freq);
	   return inputDevice;
	} catch (int e) {
	   fprintf (stderr, "no sdrplay detected, going on\n");}
#endif
#ifdef	__HAVE_AIRSPY__
	try {
	   inputDevice	= new airspyHandler (dumpSettings, freq);
	   return inputDevice;
	} catch (int e) {
	   fprintf (stderr, "no airspy detected, going on\n");
	}
#endif
#ifdef	__HAVE_RTLSDR__
	try {
	   inputDevice	= new rtlsdrHandler (dumpSettings, freq);
	   return inputDevice;
	} catch (int e) {
	   fprintf (stderr, "no rtlsdr device detected, going on\n");
	}
#endif
	return new deviceHandler ();
}


void showHelp (void) {
    printf(
"--device-index <index>   Select RTL device (default: 0).\n"
"--gain <db>              Set gain (default: max gain. Use -100 for auto-gain).\n"
"--enable-agc             Enable the Automatic Gain Control (default: off).\n"
"--freq <hz>              Set frequency (default: 1090 Mhz).\n"
"--ifile <filename>       Read data from file (use '-' for stdin).\n"
"--interactive            Interactive mode refreshing data on screen.\n"
"--interactive-rows <num> Max number of rows in interactive mode (default: 15).\n"
"--interactive-ttl <sec>  Remove from list if idle for <sec> (default: 60).\n"
"--raw                    Show only messages hex values.\n"
"--net                    Enable networking.\n"
"--net-only               Enable just networking, no RTL device or file used.\n"
"--net-ro-port <port>     TCP listening port for raw output (default: 30002).\n"
"--net-ri-port <port>     TCP listening port for raw input (default: 30001).\n"
"--net-http-port <port>   HTTP server port (default: 8080).\n"
"--net-sbs-port <port>    TCP listening port for BaseStation format output (default: 30003).\n"
"--no-fix                 Disable single-bits error correction using CRC.\n"
"--no-crc-check           Disable messages with broken CRC (discouraged).\n"
"--aggressive             More CPU for more messages (two bits fixes, ...).\n"
"--stats                  With --ifile print stats at exit. No other output.\n"
"--onlyaddr               Show only ICAO addresses (testing purposes).\n"
"--metric                 Use metric units (meters, km/h, ...).\n"
"--snip <level>           Strip IQ file removing samples < level.\n"
"--debug <flags>          Debug mode (verbose), see README for details.\n"
"--help                   Show this help.\n"
"\n"
"Debug mode flags: d = Log frames decoded with errors\n"
"                  D = Log frames decoded with zero errors\n"
"                  c = Log frames with bad CRC\n"
"                  C = Log frames with good CRC\n"
"                  p = Log frames with bad preamble\n"
"                  n = Log network debugging info\n"
"                  j = Log frames to frames.js, loadable by debug.html.\n"
    );
}
