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
	std::optional<std::tuple<P, P, V, P>> choosePoints(F func, PointRegion<P> r, std::size_t maxIter)
	{
		using std::abs;

		auto [a, fa, b, fb] = this->countBwV(func, r);
		auto x = (a + b) / 2;
		auto fx = func(x);

		if (abs(fx - fa) < epsilon || abs(fx - fb) < epsilon) return std::make_tuple(a, x, fx, b);

		for (auto min = fa < fb ? a : b; abs(x - min) >= epsilon && maxIter > 0;
				 maxIter--, x = (x + min) / 2, fx = func(x))
			if (fa >= fx && fx <= fb)
				return std::make_tuple(a, x, fx, b);
		return {};
	}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r)
	{
		BEGIN_APPROX_COROUTINE(data);

		// auto a = r.l.p, b = r.r.p;
		auto res = choosePoints(func, r, 10);
		if (!res.has_value()) {
			// cannot satisfy initial conditions
			co_yield r;
			co_return;
		}

		auto [x1, x2, f2, x3] = *res;
		auto f1 = func(x1), f3 = func(x3);

		std::optional<P> last_x_bar;

		while (true)
		{
			using std::abs;
			auto [a0, a1, a2, x_bar] = approxParabola(x1, f1, x2, f2, x3, f3);
			if (!(abs(x2 - x1) >= epsilon && abs(x3 - x2) >= epsilon && abs(x3 - x1) >= epsilon))
			{
				// cannot find minimum now
				co_yield {x1, x3, bound_tag};
				break;
			}
			if (!(abs(a2) >= epsilon))
			{
				// && f(x1) >= f(x2) <= f(x3) => f === const on [x1, x3]
				co_yield {x2, x2, bound_tag};
				break;
			}
			data->a0 = a0, data->a1 = a1, data->a2 = a2;
			data->x1 = x1, data->x2 = x2, data->x3 = x3;
			auto f_x_bar = func(x_bar);
			data->bar = {x_bar, f_x_bar};

			if (last_x_bar.has_value())
			{
				using std::abs;
				auto delta = abs(x_bar - *last_x_bar);
				if (delta < epsilon)
				{
					co_yield {x_bar, x_bar, bound_tag};
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

			co_yield {x1, x3, bound_tag};
		}
	}
};
