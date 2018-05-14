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
#include	"sdrplay-handler.h"

static
void sdrplay_dataCallback (int16_t          *xi,
	                   int16_t          *xq,
	                   uint32_t         firstSampleNum,
	                   int32_t          grChanged,
	                   int32_t          rfChanged,
                           int32_t          fsChanged,
                           uint32_t         numSamples,
                           uint32_t         reset,
                           void             *ctx) {
uint32_t	i;
sdrplayHandler *st = static_cast <sdrplayHandler *>(ctx);
int16_t localBuf [numSamples];
	MODES_NOTUSED (firstSampleNum);
	MODES_NOTUSED (grChanged);
	MODES_NOTUSED (rfChanged);
	MODES_NOTUSED (fsChanged);
	MODES_NOTUSED (reset);
//
	for (i = 0; i < numSamples; i ++) {
	   localBuf [i] = (xi [i] < 0 ? -xi [i] : xi [i]) +
	                            (xq [i] < 0 ? -xq [i] : xq [i]);
//	   localBuf [i] /= 8;
	}
//
//	if we have a buffer filled, signal the receiving thread
	st -> dataBuffer -> putDataIntoBuffer (localBuf, numSamples);
	if (st -> dataBuffer -> GetRingBufferReadAvailable () > 256000)
	   st -> signalData ();
}

void    sdrplay_changeCallback (uint32_t  gRdB,
                                uint32_t  lnaGRdB,
                                void      *cbContext) {
//	fprintf (stderr, "gain reduction set to %d\n", gRdB);
//	fprintf (stderr, "lna reduction set to %d\n",lnaGRdB);
	(void)cbContext;
}

	sdrplayHandler::sdrplayHandler (QSettings *sdrplaySettings,
	                                int	freq) {
float	version;
mir_sdr_ErrT	err;
mir_sdr_DeviceT devDesc [4];

	this	-> sdrplaySettings	= sdrplaySettings;
	this	-> freq		= freq;

	err	=  mir_sdr_ApiVersion (&version);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "Failure in getting library version\n");
	   throw (1);
	}
	
	if (version < 2.05) {
           fprintf (stderr, "sorry, library too old\n");
	   throw (1);
        }

        mir_sdr_GetDevices (devDesc, &numofDevs, (uint32_t) (4));
        if (numofDevs == 0) {
           fprintf (stderr, "Sorry, no device found\n");
	   throw (2);
        }

	deviceIndex = 0;
        hwVersion = devDesc [deviceIndex]. hwVer;
        fprintf (stderr, "sdrdevice found = %s, hw Version = %d\n",
                                      devDesc [deviceIndex]. SerNo,
	                                        hwVersion);
        mir_sdr_SetDeviceIdx (deviceIndex);
//
//	we have a device, so let's show it

	this	-> myFrame		= new QFrame (NULL);
	setupUi (this -> myFrame);
	this	-> myFrame	-> show ();
	serialNumber -> setText (devDesc [deviceIndex]. SerNo);
	antennaSelector		-> hide ();
        if (hwVersion == 2) {
           antennaSelector -> show ();
           connect (antennaSelector, SIGNAL (activated (const QString &)),
                    this, SLOT (set_antennaControl (const QString &)));
        }


	sdrplaySettings         -> beginGroup ("sdrplaySettings");
        gainSlider              -> setValue (
                    sdrplaySettings -> value ("sdrplayGain", 50). toInt ());
        ppmControl              -> setValue (
                    sdrplaySettings -> value ("sdrplay-ppm", 0). toInt ());
	enable_agc	=
	            sdrplaySettings -> value ("sdrplay-agc", 0). toInt () == 1;
        sdrplaySettings -> endGroup ();

        setExternalGain (gainSlider     -> value ());
        set_ppmControl  (ppmControl     -> value ());
	agcControl	-> setText (enable_agc ? "agc on" : "agc off");
//
//      and be prepared for future changes in the settings
        connect (gainSlider, SIGNAL (valueChanged (int)),
                 this, SLOT (setExternalGain (int)));
	connect (ppmControl, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ppmControl (int)));
        connect (agcControl, SIGNAL (clicked (void)),
                 this, SLOT (agcControl_toggled (void)));
	
	dataBuffer	= new RingBuffer<int16_t> (32 * 32768);
}

	sdrplayHandler::~sdrplayHandler (void) {
	if (numofDevs > 0)
	   mir_sdr_ReleaseDeviceIdx ();
	sdrplaySettings -> beginGroup ("sdrplaySettings");
        sdrplaySettings -> setValue ("sdrplayGain", gainSlider -> value ());
        sdrplaySettings -> setValue ("sdrplay-ppm", ppmControl -> value ());
	sdrplaySettings	-> setValue ("sdrplay-agc", enable_agc ? 1 : 0);
        sdrplaySettings -> endGroup ();
        sdrplaySettings -> sync ();
}

void	sdrplayHandler::startDevice (void) {
int   localGRed       = 102 - gainSlider -> value ();
int   gRdBSystem;
int   err;
int	samplesPerPacket;

	err  = mir_sdr_StreamInit (&localGRed,
                                   (double)(2000000) / 1000000.0,
                                   (double)(freq) / 1000000.0,
                                   mir_sdr_BW_5_000,
                                   mir_sdr_IF_Zero,
                                   1,        // lnaEnable do not know yet
                                   &gRdBSystem,
                                   mir_sdr_USE_SET_GR,
                                   &samplesPerPacket,
                                   (mir_sdr_StreamCallback_t)sdrplay_dataCallback,
                                   (mir_sdr_GainChangeCallback_t)sdrplay_changeCallback,
                                   (void *)this);
	if (err != mir_sdr_Success) {
           fprintf (stderr, "Error %d on streamInit\n", err);
           throw (3);
        }

	mir_sdr_AgcControl (enable_agc ?
	                        mir_sdr_AGC_100HZ :
	                        mir_sdr_AGC_DISABLE,
	                      - (102 - gainSlider -> value ()), 0, 0, 0, 0, 0);
        if (!enable_agc)
	   mir_sdr_SetGr (102 - gainSlider -> value (), 1, 0);

        err             = mir_sdr_SetDcMode (4, 1);
	err             = mir_sdr_SetDcTrackTime (63);
	fprintf (stderr, "gestart\n");
}

void	sdrplayHandler::stopDevice (void) {
	myFrame	-> hide ();
	mir_sdr_StreamUninit    ();
	mir_sdr_ReleaseDeviceIdx ();
}

int	sdrplayHandler::getSamples (int16_t *buffer, int amount) {
	dataBuffer	-> getDataFromBuffer (buffer, amount);
	return amount;
}

int	sdrplayHandler::Samples	(void) {
	return dataBuffer -> GetRingBufferReadAvailable ();
}

void	sdrplayHandler::signalData	(void) {
	emit dataAvailable ();
}

void    sdrplayHandler::agcControl_toggled (void) {
int	currentGred	= 102 - gainSlider -> value ();
	enable_agc = !enable_agc;
        mir_sdr_AgcControl (enable_agc ?
	                        mir_sdr_AGC_100HZ :
                                mir_sdr_AGC_DISABLE,
	                                   -currentGred, 0, 0, 0, 0, 1);
        if (!enable_agc)
           setExternalGain (gainSlider -> value ());
	agcControl -> setText (enable_agc ?"agc on" : "agc_off");
}

void    sdrplayHandler::set_ppmControl (int ppm) {
        mir_sdr_SetPpm    ((float)ppm);
        mir_sdr_SetRf     ((float)freq, 1, 0);
}

void    sdrplayHandler::set_antennaControl (const QString &s) {
mir_sdr_ErrT err;

        if (hwVersion < 2)      // should not happen
           return;

        if (s == "Antenna A")
           err = mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
        else
           err = mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_B);
}

void	sdrplayHandler::setExternalGain (int gain) {
	fprintf (stderr, "gain reduction %d\n", 102 - gain);
        (void) mir_sdr_SetGr (102 - gain, 1, 0);
        gainDisplay     -> display (gain);
}

