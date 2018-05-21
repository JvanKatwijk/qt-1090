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
#include	<getopt.h>
#include        "adsb-constants.h"
#include	"qt-1090.h"

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

int     main (int argc, char **argv) {
QString initFileName	= fullPathfor (QString (DEFAULT_INI), ".ini");
qt1090  *MyRadioInterface;
QSettings       *dumpSettings;           // ini file
int     j;
char	*fileName	= NULL;
int	deviceIndex	= 0;
int	freq		= 1090000000;
int	opt;
bool	network		= false;

	QCoreApplication::setOrganizationName	("Lazy Chair Computing");
	QCoreApplication::setOrganizationDomain ("Lazy Chair Computing");
	QCoreApplication::setApplicationName	("qt-1090");
	QCoreApplication::setApplicationVersion (QString (CURRENT_VERSION) + " Git: " + GITHASH);

//	Parse the command line options 

	while ((opt = getopt (argc, argv, "f:F:n")) != -1) {
	   switch (opt) {	// there aren't many
	      case 'f':
	      case 'F':	
	         freq	= atoi (optarg);
	         break;
	      case 'n':
	         network	= true;
	         break;
	      default:
	         break;
	   }
	}
	dumpSettings =  new QSettings (initFileName, QSettings::IniFormat);
/*
 *      Before we connect control to the gui, we have to
 *      instantiate
 */
	QApplication a (argc, argv);
//	setting the language
//	QString locale = QLocale::system (). name ();
//	qDebug() << "main:" <<  "Detected system language" << locale;
//	setTranslator (locale);

//	a. setWindowIcon (QIcon (":/dab-radio.ico"));

	MyRadioInterface = new qt1090 (dumpSettings, freq, network);
	MyRadioInterface -> show ();

#if QT_VERSION >= 0x050600
	QGuiApplication::setAttribute (Qt::AA_EnableHighDpiScaling);
#endif
        a. exec ();
/*
 *      done:
 */
	dumpSettings -> sync ();
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


