#pragma once

#include <string>
#include <concepts>
#include <optional>

#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>

#include "opt-methods/solvers/Approximator.hpp"

// namespace connected with UI drawing and qt
namespace Charting
{
	template<std::derived_from<QtCharts::QAbstractSeries> SeriesT = QtCharts::QLineSeries, typename P, std::invocable<P> Func>
	SeriesT* plotFunction(Func&& func, RangeBounds<P> r, std::size_t nOfPoints, std::string const& name)
	{
		assert(nOfPoints != 0);
		auto plot = new SeriesT();
		for (std::size_t i = 0; i < nOfPoints; i++)
		{
			using std::lerp;
			auto x = lerp(r.l, r.r, i * 1.0 / (nOfPoints - 1));
			auto y = func(x);
			if (!std::isnan(y))
				*plot << QPointF{static_cast<qreal>(x), static_cast<qreal>(y)};
		}
		if (!name.empty())
			plot->setName(name.c_str());
		return plot;
	}

	template<std::derived_from<QtCharts::QAbstractSeries> SeriesT = QtCharts::QLineSeries, typename P>
	SeriesT* plotCircular(std::invocable<P> auto&& func1, std::invocable<P> auto&& func2, RangeBounds<P> r, std::size_t nOfPoints, std::string const& name)
	{
		assert(nOfPoints != 0);
		auto plot = new SeriesT();
		using std::lerp;
		std::optional<PointAndValue<P, P>> first;
		auto addPoint = [&](P x, P y) {
			if (!std::isnan(y))
			{
				if (!first.has_value())
					first = {x, y};
				*plot << QPointF{static_cast<qreal>(x), static_cast<qreal>(y)};
			}
		};
		for (std::size_t i = 0; i < nOfPoints; i++)
		{
			auto x = lerp(r.l, r.r, i * 1.0 / (nOfPoints - 1));
			addPoint(x, func1(x));
		}
		for (std::size_t i = 0; i < nOfPoints; i++)
		{
			auto x = lerp(r.r, r.l, i * 1.0 / (nOfPoints - 1));
			addPoint(x, func2(x));
		}
		if (first.has_value())
			addPoint(first->p, first->v);
		if (!name.empty())
			plot->setName(name.c_str());
		return plot;
	}

	template<std::derived_from<QtCharts::QAbstractSeries> SeriesT = QtCharts::QScatterSeries>
	SeriesT *drawPoints(std::vector<QPointF> const& pts, std::string const& name)
	{
		auto plot = new SeriesT();
		for (auto &pt : pts)
			*plot << pt;
		plot->setName(name.c_str());
		return plot;
	}

	template<std::derived_from<QtCharts::QAbstractAxis> AxisT = QtCharts::QAbstractAxis>
	AxisT *axisX(QtCharts::QChart* chart) {
		return static_cast<AxisT *>(chart->axes(Qt::Horizontal)[0]);
	}
	template<std::derived_from<QtCharts::QAbstractAxis> AxisT = QtCharts::QAbstractAxis>
	AxisT *axisY(QtCharts::QChart* chart) {
		return static_cast<AxisT *>(chart->axes(Qt::Vertical)[0]);
	}

	inline void addToChart(QtCharts::QChart *chart, QtCharts::QAbstractSeries *s) {
		chart->addSeries(s);
		s->attachAxis(axisX(chart));
		s->attachAxis(axisY(chart));
	}

	inline qreal getAxisRange(QtCharts::QValueAxis *axis) { return axis->max() - axis->min(); }

	inline void growAxisRange(QtCharts::QValueAxis *axis, double coef)
	{
		qreal r = getAxisRange(axis);
		axis->setRange(axis->min() - r * coef, axis->max() + r * coef);
	}

	inline void createNaturalSequenceAxes(QtCharts::QChart* chart, int n) {
		using namespace QtCharts;
		chart->createDefaultAxes();
		auto* x = axisX<QtCharts::QValueAxis>(chart);
		auto* y = axisY<QtCharts::QValueAxis>(chart);

		x->setTickAnchor(1);
		x->setTickInterval(1);
		x->setTickCount(n);

		x->setLabelFormat("%i");
		growAxisRange(y, 0.01);
		y->setTickAnchor(0);
		y->setTickInterval(getAxisRange(y) / 10);
		y->setTickType(QtCharts::QValueAxis::TicksDynamic);
	}
} // namespace Charting
