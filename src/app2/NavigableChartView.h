#pragma once

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtWidgets/QRubberBand>

class NavigableChartView : public QtCharts::QChartView
{
	Q_OBJECT
public:
	NavigableChartView(QWidget *parent = nullptr);
  void setChart(QtCharts::QChart *chart, double tickInterval);

protected:
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

private:
  static constexpr qreal ZOOM_FACTOR = 0.8;
	static constexpr qreal MIN_SCALE   = 0.01;
	QPointF calc_posf(QPointF pos);

	bool left_bt_pressed;
	QPoint last_mouse_pos;
};
