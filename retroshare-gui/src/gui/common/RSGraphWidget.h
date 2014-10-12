/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2014 RetroShare Team
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#pragma once

#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>

#include <stdint.h>

#define HOR_SPC       2   /** Space between data points */
#define SCALE_WIDTH   75  /** Width of the scale */
#define MINUSER_SCALE 2000  /** 2000 users is the minimum scale */  
#define SCROLL_STEP   4   /** Horizontal change on graph update */

#define BACK_COLOR    Qt::white
#define SCALE_COLOR   Qt::black
#define GRID_COLOR    Qt::black
#define RSDHT_COLOR   Qt::magenta
#define ALLDHT_COLOR  Qt::yellow

#define FONT_SIZE     11

// This class provides a source value that the graph can retrieve on demand.
// In order to use your own source, derive from RSGraphSource and overload the value() method.
//
class RSGraphSource: public QObject
{
    Q_OBJECT

public:
    RSGraphSource();
    virtual ~RSGraphSource() ;

    void start() ;
    void stop() ;
    void clear() ;

    virtual int n_values() const =0; // Method to overload in your own class

    // Returns the n^th interpolated value at the given time in floating point seconds backward.
    virtual void getDataPoints(int index, std::vector<QPointF>& pts) const ;

    // Sets the maximum time for keeping values. Units=seconds.
    void setCollectionTimeLimit(qint64 msecs) ;

    // Sets the time period for collecting new values. Units=milliseconds.
    void setCollectionTimePeriod(qint64 msecs) ;

protected slots:
    // Calls the internal source for a new data points; called by the timer.
    void update() ;

protected:
    virtual void getValues(std::vector<float>& values) const = 0 ;	// overload this in your own class to fill in the values you want to display.

    qint64 getTime() const ;						// returns time in ms since RS has started

    std::list<std::pair<qint64,std::vector<float> > > _points ;

    QTimer *_timer ;

    qint64 _time_limit_msecs ;
    qint64 _update_period_msecs ;
    qint64 _time_orig_msecs ;
};

class RSGraphWidget: public QFrame
{
	Q_OBJECT

	public:
		static const uint32_t RSGRAPH_FLAGS_AUTO_SCALE_Y    = 0x0001 ;		// automatically adjust Y scale
		static const uint32_t RSGRAPH_FLAGS_LOG_SCALE_Y     = 0x0002 ;		// log scale in Y
		static const uint32_t RSGRAPH_FLAGS_ALWAYS_COLLECT  = 0x0004 ;		// keep collecting while not displayed

		/** Bandwidth graph style. */
		enum GraphStyle 
		{
			SolidLine = 0,  /**< Plot bandwidth as solid lines. */
			AreaGraph = 1   /**< Plot bandwidth as alpha blended area graphs. */
		};

		/** Default Constructor */
		RSGraphWidget(QWidget *parent = 0);
		/** Default Destructor */
		~RSGraphWidget();

		// sets the update interval period.
		//
		void setTimerPeriod(int miliseconds) ;				
		void addSource(RSGraphSource *gs) ;
        void setTimeScale(float pixels_per_second) ;

		/** Add data points. */
		//void addPoints(qreal rsDHT, qreal allDHT);
		/** Clears the graph. */
		void resetGraph();
		/** Toggles display of data counters. */
		//void setShowCounters(bool showRSDHT, bool showALLDHT);
		/** Sets the graph style used to display bandwidth data. */
		void setGraphStyle(GraphStyle style) { _graphStyle = style; }

    protected:
		/** Overloaded QWidget::paintEvent() */
		void paintEvent(QPaintEvent *event);

	protected slots:
        void updateDisplay() {}

	private:
		/** Gets the width of the desktop, the max # of points. */
		int getNumPoints();

		/** Paints an integral and an outline of that integral for each data set
		 * (rsdht and/or alldht) that is to be displayed. */
		void paintData();
		/** Paints the rsdht/alldht totals. */
		void paintTotals();
		/** Paints the scale in the graph. */
		void paintScale();
		/** Returns a formatted string representation of total. */
		QString totalToStr(qreal total);
		/** Returns a list of points on the bandwidth graph based on the supplied set
		 * of rsdht or alldht values. */
        void pointsFromData(QVector<QPointF> & ) ;

		/** Paints a line with the data in <b>points</b>. */
        void paintLine(const QVector<QPointF>& points, QColor color,
				Qt::PenStyle lineStyle = Qt::SolidLine);
		/** Paints an integral using the supplied data. */
        void paintIntegral(const QVector<QPointF>& points, QColor color, qreal alpha = 1.0);

		/** Style with which the bandwidth data will be graphed. */
		GraphStyle _graphStyle;
		/** A QPainter object that handles drawing the various graph elements. */
		QPainter* _painter;
		/** The current dimensions of the graph. */
		QRect _rec;
		/** The maximum data value plotted. */
		qreal _maxValue;
		/** The maximum number of points to store. */
		int _maxPoints;

        qreal _time_scale ; // horizontal scale in pixels per sec.

		/** Show the respective lines and counters. */
		//bool _showRSDHT;
		//bool _showALLDHT;

		uint32_t _flags ;
		QTimer *_timer ;

		std::vector<RSGraphSource *> _sources ;
};

