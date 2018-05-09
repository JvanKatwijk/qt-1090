
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
 *	recoding and taking parts for the airspyRadio interface
 *	for the Qt-DAB program
 *	jan van Katwijk
 *	Lazy Chair Computing
 */
#ifndef __AIRSPY_HANDLER__
#define	__AIRSPY_HANDLER__

#include	<QObject>
#include	<QSettings>
#include	<QFrame>
#include	<complex>
#include	<vector>
#include	<atomic>
#include	"adsb-constants.h"
#include	"ringbuffer.h"
#include	"device-handler.h"
#include	"ui_airspy-widget.h"
#ifndef	__MINGW32__
#include	"libairspy/airspy.h"
#else
#include	"libairspy/airspy.h"
#endif

class airspyHandler: public deviceHandler, public Ui_airspyWidget {
Q_OBJECT
public:
			airspyHandler		(QSettings *, int freq);
			~airspyHandler		(void);
	void		startDevice		(void);
	void		stopDevice		(void);
	int32_t		getSamples		(int16_t *, int32_t size);
	int32_t		Samples			(void);
	int16_t		currentTab;
private slots:
	void		set_linearity		(int value);
	void		set_sensitivity		(int value);
	void		set_lna_gain		(int value);
	void		set_mixer_gain		(int value);
	void		set_vga_gain		(int value);
	void		set_lna_agc		(void);
	void		set_mixer_agc		(void);
	void		set_rf_bias		(void);
	void		show_tab		(int);
private:
	QFrame		*myFrame;
	std::atomic<bool>	running;
	bool		lna_agc;
	bool		mixer_agc;
	bool		rf_bias;
const	char*		board_id_name (void);

	int16_t		vgaGain;
	int16_t		mixerGain;
	int16_t		lnaGain;
	int32_t		selectedRate;
	std::vector<std::complex<float>>	convBuffer;
	int16_t		convIndex;
	int16_t		convBufferSize;
	int16_t		mapTable_int   [4 * 500];
	float		mapTable_float [4 * 500];
	QSettings	*airspySettings;
	RingBuffer<int16_t> *_I_Buffer;
	int32_t		inputRate;
	bool		filtering;
	struct airspy_device* device;
	uint64_t 	serialNumber;
	char		serial[128];
static
	int		callback(airspy_transfer_t *);
	int		data_available (void *buf, int buf_size);
const	char *		getSerial (void);
	int		open (void);
};

#endif
