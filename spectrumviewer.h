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

#ifndef	__SYNC_VIEWER__
#define	__SYNC_VIEWER__


#include	<complex>
#include	<vector>
#include	<fftw3.h>
#include        <QObject>
#include        <qwt.h>
#include        <qwt_plot.h>
#include        <qwt_plot_curve.h>
#include        <qwt_plot_marker.h>
#include        <qwt_plot_grid.h>
#include        <qwt_color_map.h>
#include        <qwt_plot_picker.h>
#include        <QBrush>
#include        <QTimer>
#include        <stdint.h>

#include	"adsb-constants.h"

class spectrumViewer {
public:
				spectrumViewer	(QwtPlot *);
        			~spectrumViewer	();
	void			Display		(std::complex<float> *,
	                                               int, int);
	void			setBitDepth	(int16_t);
private:

	QColor			displayColor;
	QColor			gridColor;
	QColor			curveColor;

	int16_t			displaySize;
	int16_t			spectrumSize;
	std::complex<float>	*spectrum;
	std::vector<double>	displayBuffer;
	std::vector<float>	Window;
	fftwf_plan		plan;
	QwtPlotMarker		*Marker;
	QwtPlot			*plotgrid;
	QwtPlotGrid		*grid;
	QwtPlotCurve		*spectrumCurve;
	QBrush			*ourBrush;
	int32_t			indexforMarker;
	void			ViewSpectrum	(double *, double *, double, int);
	float			get_db		(float);
	int32_t			normalizer;
};

#endif


