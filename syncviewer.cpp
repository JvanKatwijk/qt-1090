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


	syncViewer::syncViewer (QwtPlot		*plot,
	                        uint16_t	bitstoShow) {
        plotgrid	= plot;
	this	-> bitstoShow	= bitstoShow;
	samplestoShow	= 16 + 2 * bitstoShow;
        displaySize	= 8 * samplestoShow;
	plotgrid	-> setCanvasBackground (Qt::black);
	grid		= new QwtPlotGrid;
	X_AXIS		= new double [displaySize];
	Y_AXIS		= new double [displaySize];
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
}

	syncViewer::~syncViewer (void) {
}

void	syncViewer::Display (uint16_t *mag, bool flag) {
int i, j;
double mmax	= 0;

	for (i = 0; i < 16 + 104; i ++)
	   if (mag [i] > mmax)
	      mmax = mag [i];

	SpectrumCurve -> setPen (flag ? QPen (Qt::green) : QPen (Qt::red));
        SpectrumCurve   -> setBrush (flag ? *greenBrush : *redBrush);
	for (i = -16; i < 2 * bitstoShow; i ++) {
	   for (j = 0; j < 8; j ++) {
	      X_AXIS [8 * (i + 16) + j] = (float)i  + (float)j / 8;
	      Y_AXIS [8 * (i + 16) + j] = mag [i] / mmax * 100;
	   }
	}

	plotgrid        -> setAxisScale (QwtPlot::xBottom,
                                         (double)X_AXIS [0],
                                         X_AXIS [displaySize - 2]);
        plotgrid        -> enableAxis (QwtPlot::xBottom);
        plotgrid        -> setAxisScale (QwtPlot::yLeft, 0, 100);

	Y_AXIS [0]	= 0;
	Y_AXIS [displaySize - 2] = 0;
	SpectrumCurve	-> setSamples (X_AXIS, Y_AXIS, displaySize - 1);
	plotgrid	-> replot ();
}


