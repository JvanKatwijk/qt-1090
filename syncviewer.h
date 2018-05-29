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

#ifndef	__SYNC_VIEWER__
#define	__SYNC_VIEWER__


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

class syncViewer {
public:
	syncViewer	(QwtPlot *);
        ~syncViewer	(void);
void    Display_1	(uint16_t *, int);
void    Display_2	(uint16_t *, int);
private:
        QwtPlot         *plotgrid;
        uint16_t        displaySize;
        QwtPlotGrid     *grid;
        QwtPlotCurve    *SpectrumCurve;
        QwtPlotMarker   *Marker;
	QBrush          *greenBrush;
	QBrush          *redBrush;
};

#endif


