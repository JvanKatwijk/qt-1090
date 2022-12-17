#
/*
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Computing
 *
 *    This file is part of the qt-1090 program
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

#include	"spectrumviewer.h"
#include	<QBrush>

	spectrumViewer::spectrumViewer (QwtPlot	*plot) {
QString	colorString	= "black";
bool	brush	= true;

	this	-> displaySize	= 1024;
	this	-> spectrumSize	= 4 * displaySize;
	this	-> displayBuffer. resize (displaySize);
	displayColor	= QColor (colorString);
	gridColor	= QColor ("white");
	curveColor	= QColor ("yellow");
	spectrum		= (std::complex<float> *)
	               fftwf_malloc (sizeof (fftwf_complex) * spectrumSize);
        plan    = fftwf_plan_dft_1d (spectrumSize,
                                  reinterpret_cast <fftwf_complex *>(spectrum),
                                  reinterpret_cast <fftwf_complex *>(spectrum),
                                  FFTW_FORWARD, FFTW_ESTIMATE);
	
	plotgrid		= plot;
	plotgrid		-> setCanvasBackground (displayColor);
	grid			= new QwtPlotGrid;
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
	grid	-> setMajPen (QPen(gridColor, 0, Qt::DotLine));
#else
	grid	-> setMajorPen (QPen(gridColor, 0, Qt::DotLine));
#endif
	grid	-> enableXMin (true);
	grid	-> enableYMin (true);
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
	grid	-> setMinPen (QPen(gridColor, 0, Qt::DotLine));
#else
	grid	-> setMinorPen (QPen(gridColor, 0, Qt::DotLine));
#endif
	grid	-> attach (plotgrid);

	spectrumCurve	= new QwtPlotCurve ("");
	spectrumCurve   -> setPen (QPen (curveColor, 2.0));
	spectrumCurve	-> setOrientation (Qt::Horizontal);
	spectrumCurve	-> setBaseline	(get_db (0));

	ourBrush	= new QBrush (curveColor);
	ourBrush	-> setStyle (Qt::Dense3Pattern);
	if (brush)
	   spectrumCurve	-> setBrush (*ourBrush);
	spectrumCurve	-> attach (plotgrid);
	
	Marker		= new QwtPlotMarker();
	Marker		-> setLineStyle (QwtPlotMarker::VLine);
	Marker		-> setLinePen (QPen (Qt::red));
	Marker		-> attach (plotgrid);
	plotgrid	-> enableAxis (QwtPlot::yLeft);

	Window. resize (spectrumSize);
	for (int i = 0; i < spectrumSize; i ++) 
	   Window [i] =
	        0.42 - 0.5 * cos ((2.0 * M_PI * i) / (spectrumSize - 1)) +
	              0.08 * cos ((4.0 * M_PI * i) / (spectrumSize - 1));
	setBitDepth	(12);
}

	spectrumViewer::~spectrumViewer () {
	fftwf_destroy_plan (plan);
	fftwf_free	(spectrum);
	delete		Marker;
	delete		ourBrush;
	delete		spectrumCurve;
	delete		grid;
}

//
void	spectrumViewer::Display (std::complex<float> *buf,
	                         int length, int amplification) {
int	frequency	= 1090000;
double	X_axis [displaySize];
double	Y_values [displaySize];
double	temp	= (double)INPUT_RATE / 2000 / displaySize;
int16_t	averageCount	= 5;

//	first X axis labels
	for (int i = 0; i < displaySize; i ++)
	   X_axis [i] = 
	         ((double)frequency - (double)(INPUT_RATE / 2000) +
	          (double)((i) * (double) 2 * temp)) / ((double)1000);
//
	for (int i = 0; i < spectrumSize; i ++) {
	   spectrum [i] = std::complex<float> (real (buf [i]) * Window [i] / 2048,
	                                       imag (buf [i]) * Window [i] / 2048);
	}

	fftwf_execute (plan);
//
//	and map the spectrumSize values onto displaySize elements
	for (int i = 0; i < displaySize / 2; i ++) {
	   double f	= 0;
	   for (int j = 0; j < spectrumSize / displaySize; j ++)
	      f += abs (spectrum [spectrumSize / displaySize * i + j]);

	   Y_values [displaySize / 2 + i] = 
                                 f / (spectrumSize / displaySize);
	   f = 0;
	   for (int j = 0; j < spectrumSize / displaySize; j ++)
	      f += abs (spectrum [spectrumSize / 2 +
	                             spectrumSize / displaySize * i + j]);
	   Y_values [i] = f / (spectrumSize / displaySize);
	}
//
//	average the image a little.
	for (int i = 0; i < displaySize; i ++) {
	   if (std::isnan (Y_values [i]) || std::isinf (Y_values [i]))
	      continue;

	   displayBuffer [i] = 
	          (double)(averageCount - 1) /averageCount * displayBuffer [i] +
	           1.0f / averageCount * Y_values [i];
	}

	memcpy (Y_values,
	        displayBuffer. data(), displaySize * sizeof (double));
	ViewSpectrum (X_axis, Y_values,
	              amplification,
	              frequency / 1000);
}

void	spectrumViewer::ViewSpectrum (double *X_axis,
	                              double *Y1_value,
	                              double amp,
	                              int32_t marker) {
float	amp1	= (float)amp / 100;

	amp		= amp / 100.0 * (- get_db (0));
	plotgrid	-> setAxisScale (QwtPlot::xBottom,
				         (double)X_axis [0],
				         X_axis [displaySize - 1]);
	plotgrid	-> enableAxis (QwtPlot::xBottom);
	plotgrid	-> setAxisScale (QwtPlot::yLeft,
				         get_db (0), get_db (0) + 2 * amp);
//				         get_db (0), 0); 

	for (int i = 0; i < displaySize; i ++) 
	   Y1_value [i] = get_db (amp1 * Y1_value [i]); 

	spectrumCurve	-> setBaseline (get_db (0));
	Y1_value [0]		= get_db (0);
	Y1_value [displaySize - 1] = get_db (0);

	spectrumCurve	-> setSamples (X_axis, Y1_value, displaySize);
	Marker		-> setXValue (marker);
	plotgrid	-> replot (); 
}

float	spectrumViewer::get_db (float x) {
	return 20 * log10 ((x + 1) / (float)(normalizer));
}

                      
void    spectrumViewer::setBitDepth     (int16_t d) {

        if (d < 0 || d > 32)
           d = 24;

        normalizer      = 1;
        while (-- d > 0) 
           normalizer <<= 1;

	fprintf (stderr, "normalizer = %d\n", normalizer);
}


