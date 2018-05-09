#
/*
 *    Copyright (C) 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
 *
 *    This file is part of the dump1090 program
 *
 *    dump1090 is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    dump1090 is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with dump1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include	<QThread>
#include	"rtlsdr-handler.h"
#include	"adsb-constants.h"
#include        "rtl-dongleselect.h"

#define		READLEN_DEFAULT	(2 * 8192)

static
void rtlsdrCallback (unsigned char *buf, uint32_t len, void *ctx) {
rtlsdrHandler *theStick = static_cast<rtlsdrHandler *>(ctx);
uint32_t i;
int16_t	lbuf [READLEN_DEFAULT / 2];
//	"len" denotes incoming bytes here
	if (len > READLEN_DEFAULT)
	   len = READLEN_DEFAULT;

	for (i = 0; i < len / 2; i ++) {
	   int16_t re = (int16_t)((int8_t)(buf [2 * i] - 128));
	   int16_t im = (int16_t)((int8_t)(buf [2 * i + 1] - 128));
	   lbuf [i] = (re < 0 ? -re : re) + (im < 0 ? -im : im);
	}

        theStick -> _I_Buffer -> putDataIntoBuffer (lbuf, len / 2);
        if (theStick -> _I_Buffer -> GetRingBufferReadAvailable () > 256000) {
	   theStick -> signalData ();
	}
}

class   dll_driver : public QThread {
private:
        rtlsdrHandler   *theStick;
public:

        dll_driver (rtlsdrHandler *d) {
        theStick        = d;
        start ();
        }

        ~dll_driver (void) {
        }

private:
virtual void    run (void) {
	fprintf (stderr, "here we go\n");
        rtlsdr_read_async (theStick -> theDevice,
                          &rtlsdrCallback,
                          (void *)theStick,
                          0,
                          READLEN_DEFAULT);
        }
};


	rtlsdrHandler::rtlsdrHandler (QSettings *s,
	                              int freq) {
int j;
int	deviceCount;
int	deviceIndex;
char vendor[256], product[256], serial[256];

	this	-> freq		= freq;
	workerHandle		= NULL;
	rtlsdrSettings		= s;
	deviceCount = rtlsdr_get_device_count ();
	if (deviceCount == 0) {
	   fprintf(stderr, "No supported RTLSDR devices found.\n");
	   throw (1);
	}

	deviceIndex = 0;        // default
        if (deviceCount > 1) {
           rtl_dongleSelect dongleSelector;
           for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex ++) {
              dongleSelector.
                   addtoDongleList (rtlsdr_get_device_name (deviceIndex));
           }
           deviceIndex = dongleSelector. QDialog::exec ();
        }
//

	if (rtlsdr_open (&theDevice, deviceIndex) < 0) {
	   fprintf (stderr, "Error opening the RTLSDR device: %s\n",
	                                               strerror (errno));
	   throw (1);
	}
//
//	It seems, we have a device, let's show it

	this	-> myFrame	= new QFrame (NULL);
	setupUi (this -> myFrame);
	myFrame	-> show ();

	gainsCount = rtlsdr_get_tuner_gains (theDevice, NULL);
        fprintf(stderr, "Supported gain values (%d): ", gainsCount);
        gains           = new int [gainsCount];
        gainsCount = rtlsdr_get_tuner_gains (theDevice, gains);
        for (int i = gainsCount; i > 0; i--) {
           fprintf(stderr, "%.1f ", gains [i - 1] / 10.0);
           combo_gain -> addItem (QString::number (gains [i - 1]));
        }
        fprintf(stderr, "\n");
	delete[] gains;

	rtlsdr_set_tuner_gain_mode (theDevice, 1);

	rtlsdr_set_center_freq (theDevice, freq);
	rtlsdr_set_sample_rate (theDevice, 2000000);
	rtlsdr_reset_buffer    (theDevice);
	_I_Buffer		= new RingBuffer<int16_t> (32 * 32768);
//
//	See what the saved values are and restore the GUI settings
        rtlsdrSettings  -> beginGroup ("rtlsdrSettings");
        QString temp =
	   rtlsdrSettings -> value ("externalGain", "100"). toString ();
        int k       = combo_gain -> findText (temp);
        if (k != -1) {
           combo_gain   -> setCurrentIndex (k);
        }

        temp    = rtlsdrSettings -> value ("autogain",
                                              "autogain_on"). toString ();
        k       = combo_autogain -> findText (temp);
        if (k != -1)
           combo_autogain       -> setCurrentIndex (k);

        ppm_correction  -> setValue (rtlsdrSettings -> value ("ppm_correction", 0). toInt ());
	rtlsdrSettings  -> endGroup ();

//      all sliders/values are set to previous values, now do the settings
//      based on these slider values
        rtlsdr_set_tuner_gain_mode (theDevice,
                           combo_autogain -> currentText () == "autogain_on");
        if (combo_autogain -> currentText () == "autogain_on")
           rtlsdr_set_agc_mode (theDevice, 1);
        else
           rtlsdr_set_agc_mode (theDevice, 0);
        rtlsdr_set_tuner_gain   (theDevice,
	                           combo_gain -> currentText (). toInt ());
        set_ppmCorrection       (ppm_correction -> value ());

//      and attach the buttons/sliders to the actions
        connect (combo_gain, SIGNAL (activated (const QString &)),
                 this, SLOT (set_ExternalGain (const QString &)));
        connect (combo_autogain, SIGNAL (activated (const QString &)),
                 this, SLOT (set_autogain (const QString &)));
        connect (ppm_correction, SIGNAL (valueChanged (int)),
                 this, SLOT (set_ppmCorrection  (int)));
}

	rtlsdrHandler::~rtlsdrHandler	(void) {
        rtlsdrSettings  -> beginGroup ("rtlsdrSettings");
        rtlsdrSettings  -> setValue ("externalGain",
                                              combo_gain -> currentText ());
        rtlsdrSettings  -> setValue ("autogain",
                                              combo_autogain -> currentText ());
        rtlsdrSettings  -> setValue ("ppm_correction",
                                              ppm_correction -> value ());
        rtlsdrSettings  -> sync ();
        rtlsdrSettings  -> endGroup ();

        rtlsdr_close (theDevice);
	delete	myFrame;
}

void	rtlsdrHandler::startDevice (void) {
	if (workerHandle != NULL)
	   return;

        _I_Buffer       -> FlushRingBuffer ();
        int r = rtlsdr_reset_buffer (theDevice);
        if (r < 0)
           return;

        rtlsdr_set_center_freq (theDevice, freq);
        workerHandle    = new dll_driver (this);
        rtlsdr_set_agc_mode (theDevice,
                combo_autogain -> currentText () == "autogain_on" ? 1 : 0);
        rtlsdr_set_tuner_gain (theDevice,
	                      combo_gain -> currentText (). toInt ());
}

void	rtlsdrHandler::stopDevice	(void) {
	if (workerHandle == NULL)
           return;
	rtlsdr_cancel_async (theDevice);
	while (!workerHandle -> isFinished ())
	   usleep (1000);
	delete    workerHandle;
	workerHandle = NULL;
	myFrame	-> hide ();
}

int	rtlsdrHandler::getSamples (int16_t *buffer, int amount) {
	_I_Buffer      -> getDataFromBuffer (buffer, amount);
	return amount;
}

int	rtlsdrHandler::Samples (void) {
	return _I_Buffer -> GetRingBufferReadAvailable ();
}

void	rtlsdrHandler::signalData	(void) {
	emit dataAvailable ();
}
//
//      when selecting  the gain from a table, use the table value
void    rtlsdrHandler::set_ExternalGain (const QString &gain) {
        rtlsdr_set_tuner_gain (theDevice, gain. toInt ());
}
//
void    rtlsdrHandler::set_autogain     (const QString &autogain) {
        rtlsdr_set_agc_mode (theDevice, autogain == "autogain_off" ? 0 : 1);
        rtlsdr_set_tuner_gain (theDevice,
	                       combo_gain -> currentText (). toInt ());
}
//
void    rtlsdrHandler::set_ppmCorrection        (int32_t ppm) {
        rtlsdr_set_freq_correction (theDevice, ppm);
}

