#pragma once

#include <numbers>
#include <cassert>
#include <optional>

#include <QtCharts/QLineSeries>

#include "opt-methods/solvers/BaseApproximator.hpp"
#include "opt-methods/util/Charting.hpp"

template<std::floating_point From, typename To> requires std::is_convertible_v<To, From>
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
		P x1, x2, x3;
		PointAndValue<P, V> bar;
	};

	static char const* name() noexcept { return "parabolic"; }

	P epsilon;

	ParabolicApproximator(P epsilon) : epsilon(epsilon) {}

	static void draw_impl(BoundsWithValues<P, V> r, IterationData const& data, QtCharts::QChart &chart)
	{
		P s = r.r.p - r.l.p;
		Charting::addToChart(
				&chart,
				Charting::plotFunction(
						[&](qreal x) { return data.a0 + data.a1 * (x - data.x1) + data.a2 * (x - data.x1) * (x - data.x2); },
						RangeBounds<P>{r.l.p - s * 2, r.r.p + s * 2},
						100,
						"Parabola"));
		Charting::addToChart(
				&chart,
				Charting::drawPoints(
						{{data.x1, data.a0},
						 {data.x2, data.a0 + data.a1 * (data.x2 - data.x1)},
						 {data.x3, data.a0 + data.a1 * (data.x3 - data.x1) + data.a2 * (data.x3 - data.x1) * (data.x3 - data.x2)}},
						"Parabola building points"));

		Charting::addToChart(&chart, Charting::drawPoints({{data.bar.p, data.bar.v}}, "Parabola vertex"));
	}

	static std::tuple<P, P, P, P> approxParabola(P x1, V f1, P x2, V f2, P x3, V f3)
	{
		auto a1 = (f2 - f1) / (x2 - x1), a2 = ((f3 - f1) / (x3 - x1) - a1) / (x3 - x2);
		return {f1, a1, a2, (x1 + x2 - static_cast<P>(a1 / a2)) / 2};
	}

	template<Function<P, V> F>
	ApproxGenerator<P, V> begin_impl(F func, BoundsWithValues<P, V> r, IterationData& data)
	{
		assert(r.l.p < r.r.p);

		auto a = r.l.p, b = r.r.p;
		/// TODO: choose x1,x2,x3 correctly
		auto x1 = a, x3 = b, x2 = (a + b) / 2;
		auto f1 = r.l.v, f3 = r.r.v, f2 = func(x2);

		std::optional<P> last_x_bar;

		while (true)
		{
			/// TODO: what if x1/x2/x3 stick together (division by zero)?
			auto [a0, a1, a2, x_bar] = approxParabola(x1, f1, x2, f2, x3, f3);
			data.a0 = a0, data.a1 = a1, data.a2 = a2;
			data.x1 = x1, data.x2 = x2, data.x3 = x3;
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
