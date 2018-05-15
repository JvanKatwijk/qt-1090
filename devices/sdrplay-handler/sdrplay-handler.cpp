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
	st -> _I_Buffer -> putDataIntoBuffer (localBuf, numSamples);
	if (st -> _I_Buffer -> GetRingBufferReadAvailable () > 256000)
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

	this	-> myFrame		= new QFrame (NULL);
	setupUi (this -> myFrame);
	this	-> myFrame	-> show ();
	libraryLoaded                   = false;

#ifdef  __MINGW32__
HKEY APIkey;
wchar_t APIkeyValue [256];
ULONG APIkeyValue_length = 255;

        if (RegOpenKey (HKEY_LOCAL_MACHINE,
                        TEXT("Software\\MiricsSDR\\API"),
                        &APIkey) != ERROR_SUCCESS) {
          fprintf (stderr,
                   "failed to locate API registry entry, error = %d\n",
	                                                 (int)GetLastError());
           delete myFrame;
           throw (21);
        }

        RegQueryValueEx (APIkey,
                         (wchar_t *)L"Install_Dir",
                         NULL,
                         NULL,
                         (LPBYTE)&APIkeyValue,
                         (LPDWORD)&APIkeyValue_length);
//      Ok, make explicit it is in the 64 bits section
        wchar_t *x = wcscat (APIkeyValue, (wchar_t *)L"\\x86\\mir_sdr_api.dll");
//      wchar_t *x = wcscat (APIkeyValue, (wchar_t *)L"\\x64\\mir_sdr_api.dll");
//      fprintf (stderr, "Length of APIkeyValue = %d\n", APIkeyValue_length);
//      wprintf (L"API registry entry: %s\n", APIkeyValue);
        RegCloseKey(APIkey);

	Handle  = LoadLibrary (x);
	if (Handle == NULL) {
          fprintf (stderr, "Failed to open mir_sdr_api.dll\n");
          delete myFrame;
          throw (22);
        }
#else
        Handle          = dlopen ("libusb-1.0.so", RTLD_NOW | RTLD_GLOBAL);
        Handle          = dlopen ("libmirsdrapi-rsp.so", RTLD_NOW);
        if (Handle == NULL)
           Handle       = dlopen ("libmir_sdr.so", RTLD_NOW);

        if (Handle == NULL) {
           fprintf (stderr, "error report %s\n", dlerror ());
           delete myFrame;
           throw (23);
        }
#endif

	libraryLoaded	= true;
        bool success = loadFunctions ();
        if (!success) {
#ifdef __MINGW32__
           FreeLibrary (Handle);
#else
           dlclose (Handle);
#endif
           delete myFrame;
           throw (23);
        }
	float ver;
        err	= my_mir_sdr_ApiVersion (&ver);
        if (ver < 2.05) {
           fprintf (stderr, "sorry, library too old\n");
#ifdef __MINGW32__
           FreeLibrary (Handle);
#else
           dlclose (Handle);
#endif
	   throw (1);
	}

        my_mir_sdr_GetDevices (devDesc, &numofDevs, (uint32_t) (4));
        if (numofDevs == 0) {
           fprintf (stderr, "Sorry, no device found\n");
#ifdef __MINGW32__
           FreeLibrary (Handle);
#else
           dlclose (Handle);
#endif
           delete myFrame;
           throw (25);
        }

	deviceIndex = 0;
        hwVersion = devDesc [deviceIndex]. hwVer;
        fprintf (stderr, "sdrdevice found = %s, hw Version = %d\n",
                                      devDesc [deviceIndex]. SerNo,
	                                        hwVersion);
        my_mir_sdr_SetDeviceIdx (deviceIndex);
//
//	we have a device, so let's show it

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
	
	_I_Buffer	= new RingBuffer<int16_t> (32 * 32768);
}

	sdrplayHandler::~sdrplayHandler (void) {
	if (!libraryLoaded)
	   return;
	sdrplaySettings -> beginGroup ("sdrplaySettings");
        sdrplaySettings -> setValue ("sdrplayGain", gainSlider -> value ());
        sdrplaySettings -> setValue ("sdrplay-ppm", ppmControl -> value ());
	sdrplaySettings	-> setValue ("sdrplay-agc", enable_agc ? 1 : 0);
        sdrplaySettings -> endGroup ();
        sdrplaySettings -> sync ();
	if (numofDevs > 0)
	   my_mir_sdr_ReleaseDeviceIdx (deviceIndex);
       if (_I_Buffer != NULL)
           delete _I_Buffer;
#ifdef __MINGW32__
        FreeLibrary (Handle);
#else
        dlclose (Handle);
#endif
	delete myFrame;
}

void	sdrplayHandler::startDevice (void) {
int   localGRed       = 102 - gainSlider -> value ();
int   gRdBSystem;
int   err;
int	samplesPerPacket;

	err  = my_mir_sdr_StreamInit (&localGRed,
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

	my_mir_sdr_AgcControl (enable_agc ?
	                        mir_sdr_AGC_100HZ :
	                        mir_sdr_AGC_DISABLE,
	                      - (102 - gainSlider -> value ()), 0, 0, 0, 0, 0);
        if (!enable_agc)
	   my_mir_sdr_SetGr (102 - gainSlider -> value (), 1, 0);

        err             = my_mir_sdr_SetDcMode (4, 1);
	err             = my_mir_sdr_SetDcTrackTime (63);
	fprintf (stderr, "gestart\n");
}

void	sdrplayHandler::stopDevice (void) {
	myFrame	-> hide ();
	my_mir_sdr_StreamUninit    ();
}

int	sdrplayHandler::getSamples (int16_t *buffer, int amount) {
	_I_Buffer	-> getDataFromBuffer (buffer, amount);
	return amount;
}

int	sdrplayHandler::Samples	(void) {
	return _I_Buffer -> GetRingBufferReadAvailable ();
}

void	sdrplayHandler::signalData	(void) {
	emit dataAvailable ();
}

void    sdrplayHandler::agcControl_toggled (void) {
int	currentGred	= 102 - gainSlider -> value ();
	enable_agc = !enable_agc;
        my_mir_sdr_AgcControl (enable_agc ?
	                        mir_sdr_AGC_100HZ :
                                mir_sdr_AGC_DISABLE,
	                                   -currentGred, 0, 0, 0, 0, 1);
        if (!enable_agc)
           setExternalGain (gainSlider -> value ());
	agcControl -> setText (enable_agc ?"agc on" : "agc_off");
}

void    sdrplayHandler::set_ppmControl (int ppm) {
        my_mir_sdr_SetPpm    ((float)ppm);
        my_mir_sdr_SetRf     ((float)freq, 1, 0);
}

void    sdrplayHandler::set_antennaControl (const QString &s) {
mir_sdr_ErrT err;

        if (hwVersion < 2)      // should not happen
           return;

        if (s == "Antenna A")
           err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
        else
           err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_B);
}

void	sdrplayHandler::setExternalGain (int gain) {
//	fprintf (stderr, "gain reduction %d\n", 102 - gain);
        (void) my_mir_sdr_SetGr (102 - gain, 1, 0);
        gainDisplay     -> display (gain);
}

bool	sdrplayHandler::loadFunctions	(void) {
	my_mir_sdr_StreamInit	= (pfn_mir_sdr_StreamInit)
	                    GETPROCADDRESS (this -> Handle,
	                                    "mir_sdr_StreamInit");
	if (my_mir_sdr_StreamInit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_StreamInit\n");
	   return false;
	}

	my_mir_sdr_StreamUninit	= (pfn_mir_sdr_StreamUninit)
	                    GETPROCADDRESS (this -> Handle,
	                                    "mir_sdr_StreamUninit");
	if (my_mir_sdr_StreamUninit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_StreamUninit\n");
	   return false;
	}

	my_mir_sdr_SetRf	= (pfn_mir_sdr_SetRf)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetRf");
	if (my_mir_sdr_SetRf == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetRf\n");
	   return false;
	}

	my_mir_sdr_SetFs	= (pfn_mir_sdr_SetFs)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetFs");
	if (my_mir_sdr_SetFs == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetFs\n");
	   return false;
	}

	my_mir_sdr_SetGr	= (pfn_mir_sdr_SetGr)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetGr");
	if (my_mir_sdr_SetGr == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetGr\n");
	   return false;
	}

	my_mir_sdr_SetGrParams	= (pfn_mir_sdr_SetGrParams)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetGrParams");
	if (my_mir_sdr_SetGrParams == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetGrParams\n");
	   return false;
	}

	my_mir_sdr_SetDcMode	= (pfn_mir_sdr_SetDcMode)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetDcMode");
	if (my_mir_sdr_SetDcMode == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDcMode\n");
	   return false;
	}

	my_mir_sdr_SetDcTrackTime	= (pfn_mir_sdr_SetDcTrackTime)
	                    GETPROCADDRESS (Handle, "mir_sdr_SetDcTrackTime");
	if (my_mir_sdr_SetDcTrackTime == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDcTrackTime\n");
	   return false;
	}

	my_mir_sdr_SetSyncUpdateSampleNum = (pfn_mir_sdr_SetSyncUpdateSampleNum)
	               GETPROCADDRESS (Handle, "mir_sdr_SetSyncUpdateSampleNum");
	if (my_mir_sdr_SetSyncUpdateSampleNum == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetSyncUpdateSampleNum\n");
	   return false;
	}

	my_mir_sdr_SetSyncUpdatePeriod	= (pfn_mir_sdr_SetSyncUpdatePeriod)
	                GETPROCADDRESS (Handle, "mir_sdr_SetSyncUpdatePeriod");
	if (my_mir_sdr_SetSyncUpdatePeriod == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetSyncUpdatePeriod\n");
	   return false;
	}

	my_mir_sdr_ApiVersion	= (pfn_mir_sdr_ApiVersion)
	                GETPROCADDRESS (Handle, "mir_sdr_ApiVersion");
	if (my_mir_sdr_ApiVersion == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ApiVersion\n");
	   return false;
	}

	my_mir_sdr_AgcControl	= (pfn_mir_sdr_AgcControl)
	                GETPROCADDRESS (Handle, "mir_sdr_AgcControl");
	if (my_mir_sdr_AgcControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_AgcControl\n");
	   return false;
	}

	my_mir_sdr_Reinit	= (pfn_mir_sdr_Reinit)
	                GETPROCADDRESS (Handle, "mir_sdr_Reinit");
	if (my_mir_sdr_Reinit == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_Reinit\n");
	   return false;
	}

	my_mir_sdr_SetPpm	= (pfn_mir_sdr_SetPpm)
	                GETPROCADDRESS (Handle, "mir_sdr_SetPpm");
	if (my_mir_sdr_SetPpm == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetPpm\n");
	   return false;
	}

	my_mir_sdr_DebugEnable	= (pfn_mir_sdr_DebugEnable)
	                GETPROCADDRESS (Handle, "mir_sdr_DebugEnable");
	if (my_mir_sdr_DebugEnable == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_DebugEnable\n");
	   return false;
	}

	my_mir_sdr_DCoffsetIQimbalanceControl	=
	                     (pfn_mir_sdr_DCoffsetIQimbalanceControl)
	                GETPROCADDRESS (Handle, "mir_sdr_DCoffsetIQimbalanceControl");
	if (my_mir_sdr_DCoffsetIQimbalanceControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_DCoffsetIQimbalanceControl\n");
	   return false;
	}


	my_mir_sdr_ResetUpdateFlags	= (pfn_mir_sdr_ResetUpdateFlags)
	                GETPROCADDRESS (Handle, "mir_sdr_ResetUpdateFlags");
	if (my_mir_sdr_ResetUpdateFlags == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ResetUpdateFlags\n");
	   return false;
	}

	my_mir_sdr_GetDevices		= (pfn_mir_sdr_GetDevices)
	                GETPROCADDRESS (Handle, "mir_sdr_GetDevices");
	if (my_mir_sdr_GetDevices == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetDevices");
	   return false;
	}

	my_mir_sdr_GetCurrentGain	= (pfn_mir_sdr_GetCurrentGain)
	                GETPROCADDRESS (Handle, "mir_sdr_GetCurrentGain");
	if (my_mir_sdr_GetCurrentGain == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetCurrentGain");
	   return false;
	}

	my_mir_sdr_GetHwVersion	= (pfn_mir_sdr_GetHwVersion)
	                GETPROCADDRESS (Handle, "mir_sdr_GetHwVersion");
	if (my_mir_sdr_GetHwVersion == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_GetHwVersion");
	   return false;
	}

	my_mir_sdr_RSPII_AntennaControl	= (pfn_mir_sdr_RSPII_AntennaControl)
	                GETPROCADDRESS (Handle, "mir_sdr_RSPII_AntennaControl");
	if (my_mir_sdr_RSPII_AntennaControl == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_RSPII_AntennaControl");
	   return false;
	}

	my_mir_sdr_SetDeviceIdx	= (pfn_mir_sdr_SetDeviceIdx)
	                GETPROCADDRESS (Handle, "mir_sdr_SetDeviceIdx");
	if (my_mir_sdr_SetDeviceIdx == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_SetDeviceIdx");
	   return false;
	}

	my_mir_sdr_ReleaseDeviceIdx	= (pfn_mir_sdr_ReleaseDeviceIdx)
	                GETPROCADDRESS (Handle, "mir_sdr_ReleaseDeviceIdx");
	if (my_mir_sdr_ReleaseDeviceIdx == NULL) {
	   fprintf (stderr, "Could not find mir_sdr_ReleaseDeviceIdx");
	   return false;
	}

	return true;
}
