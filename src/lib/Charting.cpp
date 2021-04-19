#include "opt-methods/util/Charting.hpp"

using namespace Charting;

void Charting::addToChart(QtCharts::QChart *chart, QtCharts::QAbstractSeries *s)
{
	s->setUseOpenGL();
	chart->addSeries(s);
	s->attachAxis(axisX(chart));
	s->attachAxis(axisY(chart));
}

qreal Charting::getAxisRange(QtCharts::QValueAxis *axis) { return axis->max() - axis->min(); }

void Charting::growAxisRange(QtCharts::QValueAxis *axis, double coef)
{
	qreal r = getAxisRange(axis);
	axis->setRange(axis->min() - r * coef, axis->max() + r * coef);
}

void Charting::createNaturalSequenceAxes(QtCharts::QChart* chart, int n)
{
	using namespace QtCharts;
	chart->createDefaultAxes();
	auto* x = axisX<QtCharts::QValueAxis>(chart);
	auto* y = axisY<QtCharts::QValueAxis>(chart);

	growAxisRange(x, 0.01);
	x->setLabelFormat("%i");
	growAxisRange(y, 0.01);
}

