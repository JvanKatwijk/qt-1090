#
/*
 *    Copyright (C) 2018
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
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
#include	"sdrplay-handler-v2.h"

static
void sdrplay_dataCallback (int16_t          *xi,
	                   int16_t          *xq,
	                   uint32_t         firstSampleNum,
	                   int32_t          grChanged,
	                   int32_t          rfChanged,
	                   int32_t          fsChanged,
	                   uint32_t         numSamples,
	                   uint32_t         reset,
	                   uint32_t	    hwRemoved,
	                   void             *ctx) {
uint32_t	i;
sdrplayHandler_v2 *st = static_cast <sdrplayHandler_v2 *>(ctx);
std::complex<float> localBuf [numSamples];
	NOTUSED (firstSampleNum);
	NOTUSED (grChanged);
	NOTUSED (rfChanged);
	NOTUSED (fsChanged);
	if (reset || hwRemoved)
	   return;
//
	for (i = 0; i < numSamples; i ++) 
	   localBuf [i] = std::complex<float> (xi [i], xq [i]);
//
//	if we have a buffer filled, signal the receiving thread
	st -> _I_Buffer. putDataIntoBuffer (localBuf, numSamples);
	if (st -> _I_Buffer. GetRingBufferReadAvailable () > 256000)
	   st -> signalData ();
}

void    sdrplay_changeCallback (uint32_t  GRdB,
	                        uint32_t  lnaGRdB,
	                        void      *cbContext) {
sdrplayHandler_v2  *p      = static_cast<sdrplayHandler_v2 *> (cbContext);
//      p -> lnaGRdBDisplay     -> display ((int)lnaGRdB);
}

static
int     RSP1_Table [] = {0, 5, 19, 24};

static
int     RSP1A_Table [] = {0, 6, 12, 20, 26, 32, 38, 43, 62};

static
int     RSP2_Table [] = {0, 5, 21, 15, 15, 34};

static
int     RSPduo_Table [] = {0, 6, 12, 20, 26, 32, 38, 43, 62};

static
int     get_lnaGRdB (int hwVersion, int lnaState) {
	switch (hwVersion) {
	   case 1:
	      return RSP1_Table [lnaState];

	   case 2:
	      return RSP2_Table [lnaState];

	   default:
	      return RSP1A_Table [lnaState];
	}
}

	sdrplayHandler_v2::sdrplayHandler_v2 (QSettings *sdrplaySettings,
	                                      int	freq) :
	                                     _I_Buffer (32 * 32768) {
float	version;
mir_sdr_ErrT	err;
mir_sdr_DeviceT devDesc [4];

	this	-> sdrplaySettings	= sdrplaySettings;
	this	-> freq		= freq;

	this	-> myFrame		= new QFrame (NULL);
	setupUi (this -> myFrame);
	this	-> myFrame	-> show ();
	antennaSelector         -> hide ();
	tunerSelector           -> hide ();

	libraryLoaded                   = false;

#ifdef  __MINGW32__
HKEY APIkey;
wchar_t APIkeyValue [256];
ULONG APIkeyValue_length = 255;

        wchar_t *libname = (wchar_t *)L"mir_sdr_api.dll";
        Handle  = LoadLibrary (libname);
        if (Handle == NULL) {
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
	   wchar_t *x =
	         wcscat (APIkeyValue, (wchar_t *)L"\\x86\\mir_sdr_api.dll");
	   RegCloseKey (APIkey);

	   Handle  = LoadLibrary (x);
	   if (Handle == NULL) {
	      fprintf (stderr, "Failed to open mir_sdr_api.dll\n");
	      delete myFrame;
	      throw (22);
	   }
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
	fprintf (stderr, "hwVer = %d\n", hwVersion);
	fprintf (stderr, "devicename = %s\n", devDesc [deviceIndex]. DevNm);

	err = my_mir_sdr_SetDeviceIdx (deviceIndex);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "error at SetDeviceIdx %s \n",
	                   errorCodes (err). toLatin1 (). data ());
#ifdef __MINGW32__
	   FreeLibrary (Handle);
#else
	   dlclose (Handle);
#endif
	   delete myFrame;
	   throw (25);
	}
//
//
//      we know we are only in the frequency range 175 .. 230 Mhz,
//      so we can rely on a single table for the lna reductions.
	switch (hwVersion) {
	   case 1:              // old RSP
	      lnaGainSetting    -> setRange (0, 3);
	      deviceLabel       -> setText ("RSP-I");
	      break;

	   case 2:
	      lnaGainSetting    -> setRange (0, 5);
	      deviceLabel       -> setText ("RSP-II");
	      antennaSelector -> show ();
	
	      err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
	      if (err != mir_sdr_Success)
	         fprintf (stderr, "error %d in setting antenna\n", err);
	      connect (antennaSelector, SIGNAL (activated (const QString &)),
	               this, SLOT (set_antennaSelect (const QString &)));
	      break;
	   case 3:
	      lnaGainSetting    -> setRange (0, 8);
	      deviceLabel       -> setText ("RSP-DUO");
	      tunerSelector        -> show ();
	      err  = my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_1);
	      if (err != mir_sdr_Success)
	         fprintf (stderr, "error %d in setting of rspDuo\n", err);
	      connect (tunerSelector, SIGNAL (activated (const QString &)),
	      this, SLOT (set_tunerSelect (const QString &)));
	      break;
	   default:
	      lnaGainSetting    -> setRange (0, 8);
	      deviceLabel       -> setText ("RSP-1A");
	      break;
	}

	sdrplaySettings         -> beginGroup ("sdrplaySettings");
	int biasT	= sdrplaySettings -> value ("biasT_selector", 0). toInt ();
	if (biasT != 0) {
           biasT_selector -> setChecked (true);
           biasT_selectorHandler (1);
        } 

	GRdBSelector            -> setValue (
	            sdrplaySettings -> value ("sdrplay-ifgrdb", 20). toInt ());
	lnaGainSetting          -> setValue (
	            sdrplaySettings -> value ("sdrplay-lnastate", 0). toInt ());

	ppmControl              -> setValue (
	            sdrplaySettings -> value ("sdrplay-ppm", 0). toInt ());
	bool    debugFlag       =
	         sdrplaySettings -> value ("sdrplay-debug", 0). toInt ();
	if (!debugFlag)
	   debugControl -> hide ();
	bool agcMode         =
	     sdrplaySettings -> value ("sdrplay-agcMode", 0). toInt () != 0;
	if (agcMode) {
	   agcControl		-> setChecked (true);
	   GRdBSelector		-> hide ();
	   gainsliderLabel      -> hide ();
	}
	sdrplaySettings -> endGroup ();

//
//      and be prepared for future changes in the settings
	connect (GRdBSelector, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ifgainReduction (int)));
	connect (lnaGainSetting, SIGNAL (valueChanged (int)),
	         this, SLOT (set_lnagainReduction (int)));
	connect (agcControl, SIGNAL (stateChanged (int)),
	         this, SLOT (agcControl_toggled (int)));
	connect (debugControl, SIGNAL (stateChanged (int)),
	         this, SLOT (debugControl_toggled (int)));
	connect (ppmControl, SIGNAL (valueChanged (int)),
	         this, SLOT (set_ppmControl (int)));
	connect (biasT_selector, SIGNAL (stateChanged (int)),
	         this, SLOT (biasT_selectorHandler (int)));

	lnaGRdBDisplay	-> display (get_lnaGRdB (hwVersion,
	                                     lnaGainSetting -> value ()));
}

	sdrplayHandler_v2::~sdrplayHandler_v2 () {
	if (!libraryLoaded)
	   return;
	sdrplaySettings -> beginGroup ("sdrplaySettings");
	sdrplaySettings -> setValue ("sdrplay-ppm", ppmControl -> value ());
	sdrplaySettings -> setValue ("sdrplay-ifgrdb",
	                                    GRdBSelector -> value ());
	sdrplaySettings -> setValue ("sdrplay-lnastate",
	                                    lnaGainSetting -> value ());
	sdrplaySettings -> setValue ("sdrplay-agcMode",
	                                  agcControl -> isChecked () ? 1 : 0);

	sdrplaySettings -> endGroup ();
	sdrplaySettings -> sync ();
	if (numofDevs > 0)
	   my_mir_sdr_ReleaseDeviceIdx (deviceIndex);
#ifdef __MINGW32__
	FreeLibrary (Handle);
#else
	dlclose (Handle);
#endif
	delete myFrame;
}

void    sdrplayHandler_v2::set_ifgainReduction     (int newGain) {
mir_sdr_ErrT    err;
int     GRdB            = GRdBSelector  -> value ();
int     lnaState        = lnaGainSetting -> value ();

	(void)newGain;

	err     =  my_mir_sdr_RSP_SetGr (GRdB, lnaState, 1, 0);
	if (err != mir_sdr_Success)
	   fprintf (stderr, "Error at set_ifgain %s\n",
	                    errorCodes (err). toLatin1 (). data ());
	else {
	   lnaGRdBDisplay       -> display (get_lnaGRdB (hwVersion, lnaState));
	}
}

void    sdrplayHandler_v2::set_lnagainReduction (int lnaState) {
mir_sdr_ErrT err;

	if (!agcControl -> isChecked ()) {
	   set_ifgainReduction (0);
	   return;
	}

	err     = my_mir_sdr_AgcControl (true, -30, 0, 0, 0, 0, lnaState);
	if (err != mir_sdr_Success)
	   fprintf (stderr, "Error at set_lnagainReduction %s\n",
	                       errorCodes (err). toLatin1 (). data ());
	else
	   lnaGRdBDisplay       -> display (get_lnaGRdB (hwVersion, lnaState));
}


void	sdrplayHandler_v2::startDevice () {
int   gRdBSystem;
mir_sdr_ErrT   err;
int	samplesPerPacket;
int     GRdB            = GRdBSelector	-> value ();
int     lnaState        = lnaGainSetting -> value ();

	err  = my_mir_sdr_StreamInit (&GRdB,
	                           (double)(2400000) / 1000000.0,
	                           (double)(freq) / 1000000.0,
	                           mir_sdr_BW_5_000,
	                           mir_sdr_IF_Zero,
	                           lnaState,
	                           &gRdBSystem,
	                           mir_sdr_USE_RSP_SET_GR,
	                           &samplesPerPacket,
	                           (mir_sdr_StreamCallback_t)sdrplay_dataCallback,
	                           (mir_sdr_GainChangeCallback_t)sdrplay_changeCallback,
	                           (void *)this);
	if (err != mir_sdr_Success) {
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
	   return;
	}
	err     = my_mir_sdr_SetPpm (double (ppmControl -> value ()));
	if (err != mir_sdr_Success)
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
	if (agcControl -> isChecked ()) {
	   my_mir_sdr_AgcControl (true,
	                          -30,
	                          0, 0, 0, 0, lnaGainSetting -> value ());
	   GRdBSelector         -> hide ();
	   gainsliderLabel      -> hide ();
	}
	err             = my_mir_sdr_SetDcMode (4, 1);
	if (err != mir_sdr_Success)
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
	err             = my_mir_sdr_SetDcTrackTime (63);
	if (err != mir_sdr_Success)
	   fprintf (stderr, "error = %s\n",
	                errorCodes (err). toLatin1 (). data ());
}

void	sdrplayHandler_v2::stopDevice () {
	myFrame	-> hide ();
	my_mir_sdr_StreamUninit    ();
}

int	sdrplayHandler_v2::getSamples (std::complex<float> *buffer,
	                                                 int amount) {
	int res = _I_Buffer. getDataFromBuffer (buffer, amount);
	return res;
}

int	sdrplayHandler_v2::Samples	() {
	return _I_Buffer. GetRingBufferReadAvailable ();
}

void	sdrplayHandler_v2::signalData	() {
	emit dataAvailable ();
}

void    sdrplayHandler_v2::debugControl_toggled (int debugMode) {
	(void)debugMode;
	my_mir_sdr_DebugEnable (debugControl -> isChecked () ? 1 : 0);
}

void    sdrplayHandler_v2::set_ppmControl (int ppm) {
	my_mir_sdr_SetPpm    ((float)ppm);
	my_mir_sdr_SetRf     ((float)freq, 1, 0);
}

void    sdrplayHandler_v2::set_antennaSelect (const QString &s) {
mir_sdr_ErrT err;

	if (hwVersion < 2)      // should not happen
	   return;

	if (s == "Antenna A")
	   err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_A);
	else
	   err = my_mir_sdr_RSPII_AntennaControl (mir_sdr_RSPII_ANTENNA_B);
}

void    sdrplayHandler_v2::set_tunerSelect (const QString &s) {
mir_sdr_ErrT err;

	if (hwVersion != 3)     // should not happen
	   return;
	if (s == "Tuner 1")
	   err  = my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_1);
	else
	   err  = my_mir_sdr_rspDuo_TunerSel (mir_sdr_rspDuo_Tuner_2);

	if (err != mir_sdr_Success)
	   fprintf (stderr, "error %d in selecting  rspDuo\n", err);
}

void    sdrplayHandler_v2::agcControl_toggled (int dummy) {
bool	agcMode      = agcControl -> isChecked () != 0;
	(void)dummy;

	my_mir_sdr_AgcControl (agcMode,
	                       -30,
	                       0, 0, 0, 0, lnaGainSetting -> value ());
	if (!agcMode) {
	   GRdBSelector         -> show ();
	   gainsliderLabel      -> show ();
	   set_ifgainReduction (0);
	}
	else {
	   GRdBSelector         -> hide ();
	   gainsliderLabel      -> hide ();
	}
}

bool	sdrplayHandler_v2::loadFunctions	() {
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

	my_mir_sdr_RSP_SetGr    = (pfn_mir_sdr_RSP_SetGr)
                            GETPROCADDRESS (Handle, "mir_sdr_RSP_SetGr");
        if (my_mir_sdr_RSP_SetGr == NULL) {
           fprintf (stderr, "Could not find mir_sdr_RSP_SetGr\n");
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

	my_mir_sdr_RSPII_BiasTControl =
	          (pfn_mir_sdr_RSPII_BiasTControl)
	                GETPROCADDRESS (Handle,  "mir_sdr_RSPII_BiasTControl");
	if (my_mir_sdr_RSPII_BiasTControl == nullptr) {
	   fprintf (stderr, "Could not find mir_sdr_RSPII_BiasTControl\n");
	   return false;
	}

	my_mir_sdr_rsp1a_BiasT =
	          (pfn_mir_sdr_rsp1a_BiasT)
	               GETPROCADDRESS (Handle, "mir_sdr_rsp1a_BiasT");
	if (my_mir_sdr_rsp1a_BiasT == nullptr) {
	   fprintf (stderr, "Could not find mir_sdr_rsp1a_BiasT\n");
	   return false;
	}

	my_mir_sdr_rspDuo_BiasT =
	          (pfn_mir_sdr_rspDuo_BiasT)
	               GETPROCADDRESS (Handle, "mir_sdr_rspDuo_BiasT");
	if (my_mir_sdr_rspDuo_BiasT == nullptr) {
	   fprintf (stderr, "Could not find mir_sdr_rspDuo_BiasT\n");
	   return false;
	}

	return true;
}

void	sdrplayHandler_v2::biasT_selectorHandler (int k) {
bool setting = biasT_selector -> isChecked ();
	sdrplaySettings -> setValue ("biasT_selector", setting ? 1 : 0);
	switch (hwVersion) {
	   case 1:		// old RSP
	      return;		// no support for biasT
	   case 2:		// RSP 2
	      my_mir_sdr_RSPII_BiasTControl (setting? 1 : 0);
	      return;
	   case 3:		// RSP duo
	      my_mir_sdr_rspDuo_BiasT (setting ? 1 : 0);
	      return;
	   default:		// RSP1a
	      my_mir_sdr_rsp1a_BiasT (setting ? 1 : 0);
	      return;
	}
}

QString sdrplayHandler_v2::errorCodes (mir_sdr_ErrT err) {
	switch (err) {
	   case mir_sdr_Success:
	      return "success";
	   case mir_sdr_Fail:
	      return "Fail";
	   case mir_sdr_InvalidParam:
	      return "invalidParam";
	   case mir_sdr_OutOfRange:
	      return "OutOfRange";
	   case mir_sdr_GainUpdateError:
	      return "GainUpdateError";
	   case mir_sdr_RfUpdateError:
	      return "RfUpdateError";
	   case mir_sdr_FsUpdateError:
	      return "FsUpdateError";
	   case mir_sdr_HwError:
	      return "HwError";
	   case mir_sdr_AliasingError:
	      return "AliasingError";
	   case mir_sdr_AlreadyInitialised:
	      return "AlreadyInitialised";
	   case mir_sdr_NotInitialised:
	      return "NotInitialised";
	   case mir_sdr_NotEnabled:
	      return "NotEnabled";
	   case mir_sdr_HwVerError:
	      return "HwVerError";
	   case mir_sdr_OutOfMemError:
	      return "OutOfMemError";
	   case mir_sdr_HwRemoved:
	      return "HwRemoved";
	   default:
	      return "???";
	}
}

int	sdrplayHandler_v2::nrBits	() {
	return 12;
}

