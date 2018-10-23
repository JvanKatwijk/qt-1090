#
/*
 *    Copyright (C) 2010, 2011, 2012
 *    Jan van Katwijk (J.vanKatwijk@gmail.com)
 *    Lazy Chair Programming
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

#include	"syncviewer.h"
#include	<QBrush>

	syncViewer::syncViewer (QwtPlot	*plot, int bitstoShow) {
        plotgrid	= plot;
	this	-> bitstoShow	= bitstoShow;
	plotgrid	-> setCanvasBackground (Qt::black);
	grid		= new QwtPlotGrid;
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
	grid	-> setMajPen (QPen(Qt::white, 0, Qt::DotLine));
#else
	grid	-> setMajorPen (QPen(Qt::white, 0, Qt::DotLine));
#endif
	grid	-> enableXMin (true);
	grid	-> enableYMin (true);
#if defined QWT_VERSION && ((QWT_VERSION >> 8) < 0x0601)
	grid	-> setMinPen (QPen(Qt::white, 0, Qt::DotLine));
#else
	grid	-> setMinorPen (QPen(Qt::white, 0, Qt::DotLine));
#endif
	grid	-> attach (plotgrid);
	SpectrumCurve   = new QwtPlotCurve ("");
        SpectrumCurve   -> setPen (QPen(Qt::white));
//      SpectrumCurve   -> setStyle     (QwtPlotCurve::Sticks);
        SpectrumCurve   -> setOrientation (Qt::Horizontal);
        SpectrumCurve   -> setBaseline  (0);
        greenBrush        = new QBrush (Qt::green);
        greenBrush        -> setStyle (Qt::Dense3Pattern);
        redBrush        = new QBrush (Qt::red);
        greenBrush        -> setStyle (Qt::Dense3Pattern);
        SpectrumCurve   -> setBrush (*redBrush);
        SpectrumCurve   -> attach (plotgrid);

	displaySize	= 16 + 2 * bitstoShow;
	X_AXIS. resize (displaySize * 8);
	Y_AXIS. resize (displaySize * 8);
}

double	x_buffer [2048];
double	y_buffer [2048];

	syncViewer::~syncViewer (void) {
}

void	syncViewer::Display_2 (uint16_t *mag, int length) {
int	i;
int	mmax	= 0;

	memmove (y_buffer, &y_buffer [length + 50],
	                          (1024 - (length + 50)) * sizeof (double));

	for (i = 0; i < 1024; i ++)
	   x_buffer [i] = i;
	for (i = 0; i < 50; i ++)
	   y_buffer [1024 - 1 - length - 50 + i] = 0;

	for (i = 0; i < length; i ++)
	   if (mag [i] > mmax)
	      mmax = mag [i];

	for (i = 0; i < length; i ++)
	   y_buffer [1024 - 1 - length + i] = mag [i] * 100 / mmax;

	SpectrumCurve -> setPen (QPen (Qt::white));
        SpectrumCurve   -> setBrush (Qt::NoBrush);
	plotgrid        -> setAxisScale (QwtPlot::xBottom,
                                         (double)x_buffer [0],
                                         x_buffer [1024 - 2]);
        plotgrid        -> enableAxis (QwtPlot::xBottom);
        plotgrid        -> setAxisScale (QwtPlot::yLeft, 0, 100);

	y_buffer [0]	= 0;
	y_buffer [1024 - 2] = 0;
	SpectrumCurve	-> setSamples (x_buffer, y_buffer, 1024 - 1);
	plotgrid	-> replot ();
}
//
//	show the samples of the prefix of a message, including the preamble
void	syncViewer::Display_1 (uint16_t *mag) {
int i, j;
double mmax	= 0;
double samples [16 + 2 * bitstoShow];
int	phase	= 5;
int	incr	= 1;
//
//	we have 6 samples on 2.4M for 5 periods of 0.5 usec

	for (i = 0; i < 16 + 2 * bitstoShow; i += 5) {
	   samples [i + 0] = 5 * mag [i + 0 + incr] / 6 +
	                     1 * mag [i + 1 + incr] / 6;
	   samples [i + 1] = 4 * mag [i + 1 + incr] / 6 +
	                     2 * mag [i + 1 + incr + 1] / 6;
	   samples [i + 2] = 3 * mag [i + 2 + incr] / 6 +
	                     3 * mag [i + 2 + incr + 1] / 6;
	   samples [i + 3] = 2 * mag [i + 3 + incr] / 6 +
	                     4 * mag [i + 3 + incr + 1] / 6;
	   samples [i + 4] = 1 * mag [i + 4 + incr] / 6 +
	                     5 * mag [i + 4 + incr + 1] / 6;
	   incr ++;
	}
	

	for (i = 0; i < 16 + 2 * bitstoShow; i ++)
	   if (samples [i] > mmax)
	      mmax = samples [i];

	for (i = -16; i < 2 * bitstoShow; i ++) {
           for (j = 0; j < 8; j ++) {
              X_AXIS [8 * (i + 16) + j] = (float)i  + (float)j / 8;
              Y_AXIS [8 * (i + 16) + j] = samples [16 + i] / mmax * 100;
           }
	}

	SpectrumCurve -> setPen (QPen (Qt::green));
        SpectrumCurve   -> setBrush (*greenBrush);
	plotgrid        -> setAxisScale (QwtPlot::xBottom,
                                         (double)X_AXIS [0],
                                         X_AXIS [displaySize * 8 - 2]);
        plotgrid        -> enableAxis (QwtPlot::xBottom);
        plotgrid        -> setAxisScale (QwtPlot::yLeft, 0, 100);

	Y_AXIS [0]	= 0;
	Y_AXIS [displaySize * 8 - 2] = 0;
	SpectrumCurve	-> setSamples (X_AXIS. data (),
	                               Y_AXIS. data (), displaySize * 8 - 1);
	plotgrid	-> replot ();
}


