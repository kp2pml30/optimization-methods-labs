#include "NavigableChartView.h"

#include <cmath>

#include <QtGui/QMouseEvent>
#include <QtCharts/QValueAxis>
#include <QGraphicsBlurEffect>

#include "opt-methods/util/Charting.hpp"

using namespace QtCharts;

NavigableChartView::NavigableChartView(QWidget* parent)
		: QChartView(parent), left_bt_pressed(false)
{
	setRenderHint(QPainter::Antialiasing);
}

QPointF NavigableChartView::calc_posf(QPointF pos)
{
	return {pos.x() * 1.0 / width(), pos.y() * 1.0 / height()};
}

void NavigableChartView::wheelEvent(QWheelEvent *event)
{
	using std::pow;
	QChartView::wheelEvent(event);

	QPointF mposf = calc_posf(event->
	// to be compatible with qt < 5.14
// #if QT_VERSION >= 0x050E00
// 		position()
// #else
		posF()
// #endif
		);
	int delta = event->angleDelta().y() / 120;
	qreal fac = pow(ZOOM_FACTOR, delta);

	QRectF screen = chart()->plotArea();
	QSizeF size = screen.size();
	{
		using namespace Charting;
		qreal curScale = std::min(getAxisRange(axisX<QValueAxis>(chart())), getAxisRange(axisY<QValueAxis>(chart())));
		if (curScale * fac < MIN_SCALE)
			fac = MIN_SCALE / curScale;
		if (fac == 1) return;
	}
	screen.moveTopLeft({screen.left() + (1 - fac) * screen.width() * mposf.x(),
											screen.top() + (1 - fac) * screen.height() * mposf.y()});
	screen.setSize(size * fac);
	chart()->zoomIn(screen);

	for (auto c : chartItems)
		c->updateGeometry();
}

static void resolveScale(QSizeF s, QChart* chart)
{
	QPointF ratio(1, 1);

	auto xaxis = Charting::axisX<QValueAxis>(chart);
	auto yaxis = Charting::axisY<QValueAxis>(chart);
	auto xrange = Charting::getAxisRange(xaxis);
	auto yrange = Charting::getAxisRange(yaxis);

	qreal fac_x = s.width() / xrange, fac_y = s.height() / yrange;
	if (fac_x > fac_y)
		ratio.setX(fac_x / fac_y);
	else
		ratio.setY(fac_y / fac_x);

	Charting::growAxisRange(xaxis, (ratio.x() - 1) / 2);
	Charting::growAxisRange(yaxis, (ratio.y() - 1) / 2);
}

void NavigableChartView::resizeEvent(QResizeEvent *event)
{
	QChartView::resizeEvent(event);

	resolveScale(chart()->plotArea().size(), chart());

	for (auto c : chartItems)
		c->updateGeometry();
}

void NavigableChartView::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		left_bt_pressed = true;
		last_mouse_pos = event->pos();
		setCursor(Qt::ClosedHandCursor);
	}

	QChartView::mousePressEvent(event);
}

void NavigableChartView::mouseMoveEvent(QMouseEvent *event)
{
	if (left_bt_pressed)
	{
		QPoint pt = event->pos();
		QPointF mdposf = pt - last_mouse_pos;
		chart()->scroll(-mdposf.x(), mdposf.y());
		last_mouse_pos = pt;
		for (auto c : chartItems)
			c->updateGeometry();
	}

	QChartView::mouseMoveEvent(event);
}

void NavigableChartView::mouseReleaseEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		left_bt_pressed = false;
		setCursor(Qt::ArrowCursor);
	}

	QChartView::mouseReleaseEvent(event);
}

void NavigableChartView::setChart(QChart* chart_)
{
	chartItems.clear();

	auto oldChart = chart();
	QChartView::setChart(chart_);
	if (oldChart != nullptr)
		delete oldChart;
	chart_->setAcceptHoverEvents(true);
	resolveScale(chart_->plotArea().size(), chart_);
	this->setMouseTracking(true);
}

void NavigableChartView::addItem(ChartItem* item)
{
	scene()->addItem(item);
	item->setChart(chart());
	chartItems.push_back(item);
	item->updateGeometry();
}
