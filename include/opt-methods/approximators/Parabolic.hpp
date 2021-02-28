#pragma once

#include <numbers>
#include <cassert>
#include <optional>

#include "opt-methods/solvers/Approximator.hpp"

template<std::floating_point From, typename To>
class ParabolicApproximator
{
private:
public:
	static char const* name() noexcept { return "parabolic"; }

	using P = From;
	using V = To;

	P epsilon;

	ParabolicApproximator(P epsilon) : epsilon(epsilon) {}

	template<Function<P, V> F>
	Generator<BoundsWithValues<P, V>> operator()(F func, BoundsWithValues<P, V> const& r)
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
					 a2 = ((f3 - f1) / (x3 - x1) - (f2 - f1) / (x2 - x1)) / (x3 - x2);
			auto x_bar = (x1 + x2 - a1 / a2) / 2;
			auto f_x_bar = func(x_bar);

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
