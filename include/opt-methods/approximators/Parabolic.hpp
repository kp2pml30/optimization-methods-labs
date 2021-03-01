#pragma once

#include <numbers>
#include <cassert>
#include <optional>

#include <QtCharts/QLineSeries>

#include "opt-methods/solvers/Approximator.hpp"

template<std::floating_point From, typename To>
class ParabolicApproximator : public BaseApproximator<From, To, ParabolicApproximator<From, To>>
{
	using BaseT = BaseApproximator<From, To, ParabolicApproximator>;
private:
public:
	using P = From;
	using V = To;

	struct IterationData : BaseT::IterationData
	{
		P a0, a1, a2;
		P x1, x2;
		PointAndValue<P, V> bar;
	};

	static char const* name() noexcept { return "parabolic"; }

	P epsilon;

	ParabolicApproximator(P epsilon) : epsilon(epsilon) {}

	void draw_impl(BoundsWithValues<P, V> r, IterationData const& data, QtCharts::QChart &chart)
	{
		using namespace QtCharts;
		{
			QLineSeries* series = new QLineSeries();
			{
				qreal l = -2, r = 2;
				auto func = [&](qreal x) {
					return data.a0 + data.a1 * (x - data.x1) + data.a2 * (x - data.x1) * (x - data.x2);
				};
				for (int i = 0; i < 500; i++)
				{
					qreal x = l + i / 500.0 * (r - l);
					QPointF p(x, func(x));
					*series << p;
				}
			}
			chart.addSeries(series);
			series->attachAxis(chart.axisX());
			series->attachAxis(chart.axisY());
		}
		{
			QScatterSeries* series = new QScatterSeries();
			*series << QPointF{data.bar.p, data.bar.v};
			chart.addSeries(series);
			series->attachAxis(chart.axisX());
			series->attachAxis(chart.axisY());
		}
	}

	template<Function<P, V> F>
	ApproxGenerator<P, V> begin_impl(F func, BoundsWithValues<P, V> r, IterationData& data)
	{
		assert(r.l.p < r.r.p);

		auto a = r.l.p, b = r.r.p;
		auto x1 = a, x3 = b, x2 = (a + b) / 2;
		auto f1 = r.l.v, f3 = r.r.v, f2 = func(x2);

		std::optional<P> last_x_bar;

		while (true)
		{
			/// TODO: what if x1/x2/x3 stick together (division by zero)?
			auto a1 = (f2 - f1) / (x2 - x1),
					 a2 = ((f3 - f1) / (x3 - x1) - a1) / (x3 - x2);
			data.a0 = f1;
			data.a1 = a1;
			data.a2 = a2;
			data.x1 = x1;
			data.x2 = x2;
			auto x_bar = (x1 + x2 - a1 / a2) / 2;
			auto f_x_bar = func(x_bar);
			data.bar = {x_bar, f_x_bar};

			if (last_x_bar.has_value())
			{
				using std::abs;
				auto delta = abs(x_bar - *last_x_bar);
				if (delta < epsilon)
				{
					co_yield {{x_bar, f_x_bar}, {x_bar, f_x_bar}};
					break;
				}
			}
			last_x_bar = x_bar;

			if (x_bar < x2)
				if (f_x_bar >= f2)
					x1 = x_bar, f1 = f_x_bar;
				else
				{
					x3 = x2, f3 = f2;
					x2 = x_bar, f2 = f_x_bar;
				}
			else
				if (f2 >= f_x_bar)
				{
					x1 = x2, f1 = f2;
					x2 = x_bar, f2 = f_x_bar;
				}
				else
					x3 = x_bar, f3 = f_x_bar;

			co_yield {{x1, f1}, {x3, f3}};
		}
	}
};
