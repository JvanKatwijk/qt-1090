
/**
 *  IW0HDV Extio
 *
 *  Copyright 2015 by Andrea Montefusco IW0HDV
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *  Some rights reserved. See COPYING, AUTHORS.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 *
 *	recoding, taking parts and extending for the airspyHandler interface
 *	for the Qt-DAB program
 *	jan van Katwijk
 *	Lazy Chair Computing
 */

#ifdef	__MINGW32__
#define	GETPROCADDRESS	GetProcAddress
#else
#define	GETPROCADDRESS	dlsym
#endif

#include	"airspy-handler.h"

static
const	int	EXTIO_NS	=  8192;
static
const	int	EXTIO_BASE_TYPE_SIZE = sizeof (float);

	airspyHandler::airspyHandler (QSettings *s, int freq) {
int	result, i;
int	distance	= 10000000;
std::vector <uint32_t> sampleRates;
uint32_t samplerateCount;

	this	-> airspySettings	= s;
	this	-> myFrame		= new QFrame (NULL);
	setupUi (this -> myFrame);
	this	-> myFrame	-> show ();

	airspySettings	-> beginGroup ("airspyHandler");
	int16_t temp 		= airspySettings -> value ("linearity", 10).
	                                                          toInt ();
	linearitySlider		-> setValue (temp);
	linearityDisplay	-> display  (temp);
	temp			= airspySettings -> value ("sensitivity", 10).
	                                                          toInt ();
	sensitivitySlider	-> setValue (temp);
	sensitivityDisplay	-> display (temp);

	vgaGain			= airspySettings -> value ("vga", 5).toInt ();
	vgaSlider		-> setValue (vgaGain);
	vgaDisplay		-> display (vgaGain);
	mixerGain		= airspySettings -> value ("mixer", 10). toInt ();
	mixerSlider		-> setValue (mixerGain);
	mixerDisplay		-> display (mixerGain);
	mixer_agc		= false;
	lnaGain			= airspySettings -> value ("lna", 5). toInt ();
	lnaSlider		-> setValue (lnaGain);
	lnaDisplay		-> display  (lnaGain);
	mixer_agc		= false;
	lna_agc			= false;
	rf_bias			= false;
	airspySettings	-> endGroup ();

	device			= 0;
	serialNumber		= 0;
	_I_Buffer		= NULL;
	strcpy (serial,"");
	result = airspy_init ();
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_init () failed: %s (%d)\n",
	             airspy_error_name ((airspy_error)result), result);
	   delete myFrame;
	   throw (21);
	}

	result = airspy_open (&device);
	if (result != AIRSPY_SUCCESS) {
	   printf ("airpsy_open () failed: %s (%d)\n",
	             airspy_error_name ((airspy_error)result), result);
	   delete myFrame;
	   throw (22);
	}

	(void) airspy_set_sample_type (device, AIRSPY_SAMPLE_INT16_IQ);
	(void) airspy_get_samplerates (device, &samplerateCount, 0);
	fprintf (stderr, "%d samplerates are supported\n", samplerateCount); 
	sampleRates. resize (samplerateCount);
	airspy_get_samplerates (device,
	                            sampleRates. data (), samplerateCount);

	selectedRate	= 0;
	for (i = 0; i < (int)samplerateCount; i ++) {
	   fprintf (stderr, "%d \n", sampleRates [i]);
	   if (abs ((int)sampleRates [i] - 2000000) < distance) {
	      distance	= abs ((int)sampleRates [i] - 2048000);
	      selectedRate = sampleRates [i];
	   }
	}

	if (selectedRate == 0) {
	   fprintf (stderr, "Sorry. cannot help you\n");
	   delete myFrame;
	   throw (23);
	}
	else
	   fprintf (stderr, "selected samplerate = %d\n", selectedRate);

	result = airspy_set_samplerate (device, selectedRate);
	if (result != AIRSPY_SUCCESS) {
           printf ("airspy_set_samplerate() failed: %s (%d)\n",
	             airspy_error_name ((enum airspy_error)result), result);
	   delete myFrame;
	   throw (24);
	}

//	The sizes of the mapTables follow from the input and output rate
//	(selectedRate / 1000) vs (2000000 / 1000)
//	so we end up with buffers with 1 msec content
	convBufferSize		= selectedRate / 1000;
	for (i = 0; i < 2000; i ++) {
	   float inVal	= float (selectedRate / 1000);
	   mapTable_int [i]	=  int (floor (i * (inVal / 2000.0)));
	   mapTable_float [i]	= i * (inVal / 2000.0) - mapTable_int [i];
	}
	convIndex	= 0;
	convBuffer. resize (convBufferSize + 1);

	_I_Buffer	= new RingBuffer<int16_t> (256 * 1024);
	tabWidget	-> setCurrentIndex (0);
	connect (linearitySlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_linearity (int)));
	connect (sensitivitySlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_sensitivity (int)));
	connect (lnaSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_lna_gain (int)));
	connect (vgaSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_vga_gain (int)));
	connect (mixerSlider, SIGNAL (valueChanged (int)),
	         this, SLOT (set_mixer_gain (int)));
	connect (lnaButton, SIGNAL (clicked (void)),
	         this, SLOT (set_lna_agc (void)));
	connect (mixerButton, SIGNAL (clicked (void)),
	         this, SLOT (set_mixer_agc (void)));
	connect (biasButton, SIGNAL (clicked (void)),
	         this, SLOT (set_rf_bias (void)));
	connect (tabWidget, SIGNAL (currentChanged (int)),
	         this, SLOT (show_tab (int)));
	displaySerial	-> setText (getSerial ());
	running. store (false);
	show_tab (0);			// will set currentTab
	(void)airspy_set_freq (device, freq);
}

	airspyHandler::~airspyHandler (void) {
	airspySettings	-> beginGroup ("airspyHandler");
	airspySettings -> setValue ("linearity", linearitySlider -> value ());
	airspySettings -> setValue ("sensitivity", sensitivitySlider -> value ());
	airspySettings -> setValue ("vga", vgaGain);
	airspySettings -> setValue ("mixer", mixerGain);
	airspySettings -> setValue ("lna", lnaGain);
	airspySettings	-> endGroup ();
	if (device != NULL) {
	   int result = airspy_close (device);
	   if (result != AIRSPY_SUCCESS) {
	      printf ("airspy_close () failed: %s (%d)\n",
	             airspy_error_name ((airspy_error)result), result);
	   }
	}
	delete myFrame;
	airspy_exit ();
err:
	if (_I_Buffer != NULL)
	   delete _I_Buffer;
}

void	airspyHandler::startDevice	(void) {
int	result;
int32_t	bufSize	= EXTIO_NS * EXTIO_BASE_TYPE_SIZE * 2;

	_I_Buffer	-> FlushRingBuffer ();
	result = airspy_set_sample_type (device, AIRSPY_SAMPLE_INT16_IQ);
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_sample_type () failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	   return;
	}

	if (currentTab == 0)
	   set_sensitivity	(sensitivitySlider -> value ());
	else 
	if (currentTab	== 1)
	   set_linearity	(linearitySlider -> value ());
	else {
	   set_vga_gain		(vgaGain);
	   set_mixer_gain	(mixerGain);
	   set_lna_gain		(lnaGain);
	}
	
	result = airspy_start_rx (device,
	            (airspy_sample_block_cb_fn)callback, this);
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_start_rx () failed: %s (%d)\n",
	         airspy_error_name ((airspy_error)result), result);
	   return;
	}
}

void	airspyHandler::stopDevice (void) {
int	result;
	myFrame	-> hide ();
	result = airspy_stop_rx (device);

	if (result != AIRSPY_SUCCESS ) 
	   printf ("airspy_stop_rx() failed: %s (%d)\n",
	          airspy_error_name ((airspy_error)result), result);
}
//
//	Directly copied from the airspy extio dll from Andrea Montefusco
int airspyHandler::callback (airspy_transfer* transfer) {
airspyHandler *p;

	if (!transfer)
	   return 0;		// should not happen
	p = static_cast<airspyHandler *> (transfer -> ctx);

// we read  AIRSPY_SAMPLE_INT16_IQ:
	int32_t bytes_to_write = transfer -> sample_count * sizeof (int16_t) * 2; 
	uint8_t *pt_rx_buffer   = (uint8_t *)transfer->samples;
	p -> data_available (pt_rx_buffer, bytes_to_write);
	return 0;
}

static inline
std::complex<float> cmul (std::complex<float> x, float y) {
	return std::complex<float> (real (x) * y, imag (x) * y);
}

//	called from AIRSPY data callback
//	2*2 = 4 bytes for sample, as per AirSpy USB data stream format
//	we do the rate conversion here, read in groups of 2 * xxx samples
//	and transform them into groups of 2 * 512 samples
int 	airspyHandler::data_available (void *buf, int buf_size) {	
int16_t	*sbuf	= (int16_t *)buf;
int nSamples	= buf_size / (sizeof (int16_t) * 2);
int16_t temp [2000];
int32_t  i, j;

	for (i = 0; i < nSamples; i ++) {
	   convBuffer [convIndex ++] = std::complex<float> (
	                                        sbuf [2 * i],
	                                        sbuf [2 * i + 1]);
	   if (convIndex > convBufferSize) {
	      for (j = 0; j < 2000; j ++) {
	         std::complex<float> xx;
	         int16_t  inpBase	= mapTable_int [j];
	         float    inpRatio	= mapTable_float [j];
	         xx	= cmul (convBuffer [inpBase + 1], inpRatio) + 
	                             cmul (convBuffer [inpBase], 1 - inpRatio);
	         temp [j] = (int)abs (xx);
	      }

	      _I_Buffer	-> putDataIntoBuffer (temp, 2000);
//
//	shift the sample at the end to the beginning, it is needed
//	as the starting sample for the next time
	      convBuffer [0] = convBuffer [convBufferSize];
	      convIndex = 1;
	   }
	}
	if (_I_Buffer -> GetRingBufferReadAvailable () >= 256000)
	   emit dataAvailable ();
	return 0;
}
//
const char *airspyHandler::getSerial (void) {
airspy_read_partid_serialno_t read_partid_serialno;
int result = airspy_board_partid_serialno_read (device,
	                                          &read_partid_serialno);
	if (result != AIRSPY_SUCCESS) {
	   printf ("failed: %s (%d)\n",
	         airspy_error_name ((airspy_error)result), result);
	   return "UNKNOWN";
	} else {
	   snprintf (serial, sizeof(serial), "%08X%08X", 
	             read_partid_serialno. serial_no [2],
	             read_partid_serialno. serial_no [3]);
	}
	return serial;
}
//
//	not used here
int	airspyHandler::open (void) {
int result = airspy_open (&device);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_open() failed: %s (%d)\n",
	          airspy_error_name ((airspy_error)result), result);
	   return -1;
	} else {
	   return 0;
	}
}

int32_t	airspyHandler::getSamples (int16_t *v, int32_t size) {

	return _I_Buffer	-> getDataFromBuffer (v, size);
}

int32_t	airspyHandler::Samples	(void) {
	return _I_Buffer	-> GetRingBufferReadAvailable ();
}
//
#define GAIN_COUNT (22)

uint8_t airspy_linearity_vga_gains[GAIN_COUNT] = { 13, 12, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 9, 8, 7, 6, 5, 4 };
uint8_t airspy_linearity_mixer_gains[GAIN_COUNT] = { 12, 12, 11, 9, 8, 7, 6, 6, 5, 0, 0, 1, 0, 0, 2, 2, 1, 1, 1, 1, 0, 0 };
uint8_t airspy_linearity_lna_gains[GAIN_COUNT] = { 14, 14, 14, 13, 12, 10, 9, 9, 8, 9, 8, 6, 5, 3, 1, 0, 0, 0, 0, 0, 0, 0 };
uint8_t airspy_sensitivity_vga_gains[GAIN_COUNT] = { 13, 12, 11, 10, 9, 8, 7, 6, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4, 4 };
uint8_t airspy_sensitivity_mixer_gains[GAIN_COUNT] = { 12, 12, 12, 12, 11, 10, 10, 9, 9, 8, 7, 4, 4, 4, 3, 2, 2, 1, 0, 0, 0, 0 };
uint8_t airspy_sensitivity_lna_gains[GAIN_COUNT] = { 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 12, 12, 9, 9, 8, 7, 6, 5, 3, 2, 1, 0 };

void	airspyHandler::set_linearity (int value) {
int	result = airspy_set_linearity_gain (device, value);
int	temp;
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_lna_gain () failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	   return;
	}
	linearityDisplay	-> display (value);
	temp	= airspy_linearity_lna_gains [GAIN_COUNT - 1 - value];
	linearity_lnaDisplay	-> display (temp);
	temp	= airspy_linearity_mixer_gains [GAIN_COUNT - 1 - value];
	linearity_mixerDisplay	-> display (temp);
	temp	= airspy_linearity_vga_gains [GAIN_COUNT - 1 - value];
	linearity_vgaDisplay	-> display (temp);
}

void	airspyHandler::set_sensitivity (int value) {
int	result = airspy_set_sensitivity_gain (device, value);
int	temp;
	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_mixer_gain() failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	   return;
	}
	sensitivityDisplay	-> display (value);
	temp	= airspy_sensitivity_lna_gains [GAIN_COUNT - 1 - value];
	sensitivity_lnaDisplay	-> display (temp);
	temp	= airspy_sensitivity_mixer_gains [GAIN_COUNT - 1 - value];
	sensitivity_mixerDisplay	-> display (temp);
	temp	= airspy_sensitivity_vga_gains [GAIN_COUNT - 1 - value];
	sensitivity_vgaDisplay	-> display (temp);
}

//

//	Original functions from the airspy extio dll
/* Parameter value shall be between 0 and 15 */
void	airspyHandler::set_lna_gain (int value) {
int result = airspy_set_lna_gain (device, lnaGain = value);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_lna_gain () failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	}
	else
	   lnaDisplay	-> display (value);
}

/* Parameter value shall be between 0 and 15 */
void	airspyHandler::set_mixer_gain (int value) {
int result = airspy_set_mixer_gain (device, mixerGain = value);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_mixer_gain() failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	}
	else
	   mixerDisplay	-> display (value);
}

/* Parameter value shall be between 0 and 15 */
void	airspyHandler::set_vga_gain (int value) {
int result = airspy_set_vga_gain (device, vgaGain = value);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_vga_gain () failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	}
	else
	   vgaDisplay	-> display (value);
}
//
//	agc's
/* Parameter value:
	0=Disable LNA Automatic Gain Control
	1=Enable LNA Automatic Gain Control
*/
void	airspyHandler::set_lna_agc (void) {
	lna_agc	= !lna_agc;
int result = airspy_set_lna_agc (device, lna_agc ? 1 : 0);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_lna_agc() failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	}
	if (lna_agc)
	   lnaButton	-> setText ("lna agc on");
	else
	   lnaButton	-> setText ("lna agc");
}

/* Parameter value:
	0=Disable MIXER Automatic Gain Control
	1=Enable MIXER Automatic Gain Control
*/
void	airspyHandler::set_mixer_agc (void) {
	mixer_agc	= !mixer_agc;

int result = airspy_set_mixer_agc (device, mixer_agc ? 1 : 0);

	if (result != AIRSPY_SUCCESS) {
	   printf ("airspy_set_mixer_agc () failed: %s (%d)\n",
	            airspy_error_name ((airspy_error)result), result);
	}
	if (mixer_agc)
	   mixerButton	-> setText ("mixer agc on");
	else
	   mixerButton	-> setText ("mixer agc");
}


/* Parameter value shall be 0=Disable BiasT or 1=Enable BiasT */
void	airspyHandler::set_rf_bias (void) {
	rf_bias	= !rf_bias;
int result = airspy_set_rf_bias (device, rf_bias ? 1 : 0);

	if (result != AIRSPY_SUCCESS) {
	   printf("airspy_set_rf_bias() failed: %s (%d)\n",
	           airspy_error_name ((airspy_error)result), result);
	}
}


const char* airspyHandler::board_id_name (void) {
uint8_t bid;

	if (airspy_board_id_read (device, &bid) == AIRSPY_SUCCESS)
	   return airspy_board_id_name ((airspy_board_id)bid);
	else
	   return "UNKNOWN";
}

void	airspyHandler::show_tab (int t) {
	if (t == 0)		// sensitivity
	   set_sensitivity	(sensitivitySlider -> value ());
	else
	if (t == 1)		// linearity
	   set_linearity	(linearitySlider -> value ());
	else {			// classic view
	   set_vga_gain		(vgaGain);
	   set_mixer_gain	(mixerGain);
	   set_lna_gain		(lnaGain);
	}
	currentTab	= t;
}

