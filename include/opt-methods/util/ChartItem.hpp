#pragma once

#include <numbers>
#include <memory>

#include <QtGui/QPainter>
#include <QtCharts/QChart>
#include <QtWidgets/QGraphicsItem>
#include <QtWidgets/QGraphicsLineItem>
#include <QFontDatabase>

class ChartItem : public QGraphicsItem
{
public:
	ChartItem(QGraphicsItem* item, QPointF translate = {}, qreal maxScale = 100, QtCharts::QChart* parent = nullptr);
	void setChart(QtCharts::QChart* chart);
	QRectF boundingRect() const override;
	QPainterPath shape() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}
	void setColor(QColor color);

public slots:
	void updateGeometry();

protected:

	std::unique_ptr<QGraphicsItem> item;
	QPointF translate;
	qreal maxScale;
	QtCharts::QChart* chart;
};
