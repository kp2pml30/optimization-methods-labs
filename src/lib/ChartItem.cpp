#include <numbers>

#include <QAbstractGraphicsShapeItem>
#include <QGraphicsLineItem>
#include <QGraphicsObject>
#include <QGraphicsScene>

#include "opt-methods/util/ChartItem.hpp"

using namespace QtCharts;

ChartItem::ChartItem(QGraphicsItem* item, QPointF translate, qreal maxScale, QChart* parent)
: item(item), maxScale(maxScale), translate(std::move(translate)), chart(parent)
{
	setZValue(11);
	setFlag(QGraphicsItem::ItemClipsChildrenToShape);
	item->setParentItem(this);
}

void ChartItem::setChart(QChart* chart)
{
	setParentItem(chart);
	this->chart = chart;
}

QRectF ChartItem::boundingRect() const
{
	return item->itemTransform(this).mapRect(item->boundingRect()) & mapRectFromScene(chart->plotArea());
}

QPainterPath ChartItem::shape() const
{
	QPainterPath intersect = QPainterPath();
	intersect.addRect(mapRectFromScene(chart->plotArea()));
	return item->itemTransform(this).map(item->shape()) & intersect;
}

void ChartItem::setColor(QColor color)
{
	if (auto shapeItem = dynamic_cast<QAbstractGraphicsShapeItem*>(item.get()))
	{
		auto brush = shapeItem->brush();
		brush.setColor(color);
		shapeItem->setBrush(brush);
		auto pen = shapeItem->pen();
		pen.setColor(color);
		shapeItem->setPen(pen);
	}
	else if (auto lineItem = dynamic_cast<QGraphicsLineItem*>(item.get()))
	{
		auto pen = lineItem->pen();
		pen.setColor(color);
		lineItem->setPen(pen);
	}
	else if (auto textItem = dynamic_cast<QGraphicsTextItem*>(item.get()))
	{
		textItem->setDefaultTextColor(color);
	}
}

void ChartItem::updateGeometry()
{
	// assume no rotation and mirror along y-axis
	// (scene coordinates increase to right/down, chart coordinates increase to right/up)
	auto zero = QPointF{0.0, 0.0}, e1 = QPointF{1.0, 0.0}, e2 = QPointF{0.0, 1.0};
	auto zero_ = chart->mapToPosition(zero),
	     e1_   = chart->mapToPosition(e1) - zero_,
	     e2_   = chart->mapToPosition(e2) - zero_;
	auto scalex = e1_.x(), scaley = -e2_.y();
	setTransform(
		QTransform::fromScale(std::min<qreal>(maxScale, scalex), std::min<qreal>(maxScale, scaley)) *
		QTransform::fromTranslate(translate.x() * scalex, translate.y() * scaley) *
		QTransform::fromScale(1, -1) *
		QTransform::fromTranslate(zero_.x(), zero_.y())
	);
}
