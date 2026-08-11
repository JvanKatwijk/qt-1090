#
/*
 *    Copyright (C)   2026
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of qt-1090
 *
 *    qt-1090 1is free software; you can redistribute it and/or modify
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
 *    along with 1090; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
//
//	The start up sequence consists of two parts,
//	* establishing the connection,
//	* verifying that the samplerate is usable (i.e. convertable to 2400000)
//	if completed we send a signal to the parent with the result

//
//	runMode is set if Qt-DAB issues a restart request,
//	and stopped when a "stop" request is issued
#include	<QJsonDocument>
#include	<QJsonObject>
#include	"packet-handler.h"
#include	"adsb-constants.h"
//
//	the messageHandler checks that the samples are read with the
//	correct samplerate.
//	Since the current version of SDRconnect passes 2M samples
//	and we need 2048000, we do a simple linear rate conversion


	packetHandler::packetHandler (QSettings	*s,
	                              const QString &hostAddress,
	                              int	portNumber,
	                              int	startFreq,
	                              RingBuffer<std::complex<float>> *b):
	                                  socketHandler (hostAddress,
	                                                   portNumber) {
	this	-> settings	= s;
	this	-> vfoFrequency	= startFreq;
	_O_Buffer		= b;
	connect (this, &socketHandler::reportConnect,
	         this, &packetHandler::connection_set);
	connect (this, &socketHandler::reportDisconnect,
	         this, &packetHandler::no_connection);
	connect (this, &socketHandler::reportStatus,
	         this, &packetHandler::eval_status);

	runMode				= false;
	outputRate			= 2400000;	// default
}

	packetHandler::~packetHandler	() {
	if (runMode)
	   iqStreamEnable (false);
}

void	packetHandler::connection_set	() {
	disconnect (this, &socketHandler::reportConnect,
	            this, &packetHandler::connection_set);
	connect (this, &socketHandler::dispatchMessage,
                 this, &packetHandler::dispatchMessage);
	connect (this, &socketHandler::binDataAvailable,
                 this, &packetHandler::binDataAvailable);
	setProperty ("device_center_frequency", QString::number (vfoFrequency));
	setProperty ("device_vfo_frequency", QString::number (vfoFrequency));
	askProperty ("device_sample_rate");
}

void	packetHandler::no_connection	() {
	emit connection_failed	();
}

void	packetHandler::iqStreamEnable	(bool b) {
QJsonObject theMessage;
	theMessage ["event_type"]	= QString ("iq_stream_enable");
	theMessage ["property"]		= QString ("");
	theMessage ["value"]		= QString (b ? "true" : "false");
	sendMessage (theMessage);
	runMode	= true;
}

//	Transfer is in segments of 1 msec
void	packetHandler::binDataAvailable () {
	std::complex<int16_t>  *inBuffer = dynVec (std::complex<int16_t>,
	                                            theSamplerate / 1000);
	std::complex<float> *outBuffer	= dynVec (std::complex<float>,
	                                            outputRate / 1000);
	while (_I_Buffer. GetRingBufferReadAvailable () >=
	                                 (uint32_t)theSamplerate / 1000) {
	   _I_Buffer. getDataFromBuffer (inBuffer, theSamplerate / 1000);
	   if (!runMode)	// only deal with data when processing is on
	      continue;
	   for (int i = 0; i < theSamplerate / 1000; i ++) {
	      convBuffer [convIndex. load ()] =
	                std::complex<float> (real (inBuffer [i]) ,
	                                     imag (inBuffer [i]) );
	      convIndex. store (convIndex. load () + 1);
	      if (convIndex. load () > convBufferSize) {
	         for (int j = 0; j < outputRate / 1000; j ++) {
                    int16_t  inpBase    = mapTable_int [j];
                    float    inpRatio   = mapTable_float [j];
                    outBuffer [j]    = convBuffer [inpBase + 1] * inpRatio +
                                       convBuffer [inpBase] * (1 - inpRatio);
	         }
                 _O_Buffer ->  putDataIntoBuffer (outBuffer, outputRate / 1000);
                 convBuffer [0] = convBuffer [convBufferSize];
                 convIndex. store (1);
	      }
	      
	   }
	   if (_O_Buffer -> GetRingBufferReadAvailable () > outputRate / 8) 
	      emit dataAvailable (1);
	}
}
//
//	messages sent by the SDRconnect are dispatched here
void	packetHandler::dispatchMessage	(const QString &m) {
	QJsonObject obj;
	QJsonDocument doc = QJsonDocument::fromJson (m. toUtf8 ());
	if (doc. isNull ())
	   return;	// cannot handle
	obj	= doc. object ();

	QString eventType	= obj ["event_type"]. toString ();
	if (eventType == "get_property_response") {
	   QString property = obj ["property"]. toString ();
	   if (property == "device_sample_rate") {
	      QString samplerate = obj ["value"]. toString ();
	      bool b;
	      double rate	= samplerate. toDouble (&b);
	      if (!b)
	         return;
//	we expect 2000000 and do not process (much) lower/higher rates
	      if ((rate > 2500000) || (rate < 1500000)) {
	         emit rateError ();
	         return;
	      }
	      else {
	         setProperty ("filter_bandwidth", "153600");
	         theSamplerate	= (int)rate;	
//	we process buffers with 1 msec content
	         convBufferSize          = theSamplerate / 1000;
	         float samplesPerMsec    = outputRate / 1000.0;
	         for (int i = 0; i < outputRate / 1000; i ++) {
	            float inVal  = float (theSamplerate / 1000);
	            mapTable_int [i]     =
	                      int (floor (i * (inVal / samplesPerMsec)));
	            mapTable_float [i]   =
	                      i * (inVal / samplesPerMsec) - mapTable_int [i];
	         }       
	         convIndex. store (0);
	         convBuffer. resize (convBufferSize + 1);
	      }
	      emit rateOK (theSamplerate, outputRate);
	      iqStreamEnable (true);  // here it really starts
	   }
	}
	if (eventType == "property_changed") {
	   QString property = obj ["property"]. toString ();
	   if (property == "device_vfo_frequency") {
	      QString vfoString = obj ["value"]. toString ();
	      bool b;
	      int vfo	= vfoString. toInt (&b);
	      if (!b)
	         return;
	      vfoFrequency	= vfo;
	      emit frequency_changed (vfo);
	   }
	   if (property == "signal_power") {
	      QString snrString = obj ["value"]. toString ();
	      QString res;
	      for (int i = 0; i < snrString. size (); i ++)
                  if (snrString. at (i) == QChar (','))
                     res. push_back (QChar ('.'));
                  else
                     res. push_back (snrString. at (i));
	      bool b;
	      double snr = res. toDouble (&b);
	      if (!b)
	         return;
	      emit signalPower (snr);
	   }
	}
}

void	packetHandler::setProperty	(const QString prop,
	                                        const QString val) {
QJsonObject theMessage;
	theMessage ["event_type"]	= QString ("set_property");
	theMessage ["property"]		= prop;
	theMessage ["value"]		= val;
	sendMessage (theMessage);
}

void	packetHandler::askProperty (const QString prop) {
QJsonObject theMessage;
	theMessage ["event_type"]	= QString ("get_property");
	theMessage ["property"]		= prop;
	sendMessage (theMessage);
}

void	packetHandler::eval_status	(int status) {
	send_status (status);
}


