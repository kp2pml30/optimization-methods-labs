#include "NavigableChartView.h"

#include <QtGui/QMouseEvent>
#include <QtCharts/QValueAxis>

#include "opt-methods/util/Charting.hpp"

QT_CHARTS_USE_NAMESPACE

NavigableChartView::NavigableChartView(QWidget *parent)
		: QChartView(parent), left_bt_pressed(false)
{
}

QPointF NavigableChartView::calc_posf(QPointF pos)
{
	return {pos.x() * 1.0 / width(), pos.y() * 1.0 / height()};
}

void NavigableChartView::wheelEvent(QWheelEvent *event)
{
	QPointF mposf = calc_posf(event->position());
	int delta = event->angleDelta().y() / 120;

	qreal fac = pow(ZOOM_FACTOR, delta);

	QRectF screen = chart()->plotArea();
	QSizeF size = screen.size();

	{
		using namespace Charting;
		if (qreal s = std::min(getAxisRange(axisX<QValueAxis>(chart())), getAxisRange(axisY<QValueAxis>(chart()))) * fac;
				s < MIN_SCALE)
			fac = MIN_SCALE / s;
		if (fac == 1) return;
	}

	screen.moveTopLeft({screen.left() + (1 - fac) * screen.width() * mposf.x(),
											screen.top() + (1 - fac) * screen.height() * mposf.y()});
	screen.setSize(size * fac);

	chart()->zoomIn(screen);

	QChartView::wheelEvent(event);
}

void NavigableChartView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		left_bt_pressed = true;
		last_mouse_pos = event->pos();
		setCursor(Qt::ClosedHandCursor);
		event->accept();
	}
	else
		event->ignore();
}

void NavigableChartView::mouseMoveEvent(QMouseEvent *event)
{
	if (left_bt_pressed)
	{
		QPoint pt = event->pos();
		QPointF mdposf = pt - last_mouse_pos;
		chart()->scroll(-mdposf.x(), mdposf.y());
		last_mouse_pos = pt;
		event->accept();
	}
	else
		event->ignore();
}

void NavigableChartView::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		left_bt_pressed = false;
		setCursor(Qt::ArrowCursor);
		event->accept();
	}
	else
		event->ignore();
}

void NavigableChartView::setChart(QtCharts::QChart *chart, double tickInterval)
{
	using namespace QtCharts;
	QChartView::setChart(chart);
	auto *x = Charting::axisX<QValueAxis>(chart);
	auto *y = Charting::axisY<QValueAxis>(chart);
	x->setTickAnchor(0);
	x->setTickInterval(tickInterval);
	x->setTickType(QtCharts::QValueAxis::TicksDynamic);
	y->setTickAnchor(0);
	y->setTickInterval(tickInterval);
	y->setTickType(QtCharts::QValueAxis::TicksDynamic);
}
