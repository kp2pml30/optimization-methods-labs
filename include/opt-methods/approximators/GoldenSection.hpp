#pragma once

#include <numbers>
#include <cassert>

#include "opt-methods/solvers/Approximator.hpp"

template<std::floating_point From, typename To>
class GoldenSectionApproximtor
{
private:
public:
	static char const* name() noexcept { return "golden section"; }

	using P = From;
	using V = To;

	static constexpr P tau = std::numbers::phi_v<P> - 1;

	template<Function<P, V> F>
	Generator<BoundsWithValues<P, V>> operator()(F func, BoundsWithValues<P, V> const& r)
	{
		assert(r.l.p < r.r.p);

		auto a = r.l.p, b = r.r.p;
		auto fa = r.l.v, fb = r.r.v;

		P x1 = a + (1 - tau) * (b - a), x2 = a + tau * (b - a);
		V f1 = func(x1), f2 = func(x2);

		while (true) {
			if (f1 < f2) {
				b = x2, fb = f2;
				x2 = x1, f2 = f1;
				x1 = a + (1 - tau) * (b - a), f1 = func(x1);
			} else {
				a = x1, fa = f1;
				x1 = x2, f1 = f2;
				x2 = a + tau * (b - a), f2 = func(x2);
			}

			co_yield {{a, fa}, {b, fb}};
		}
	}
};
