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

	bool isFirst = true;
	V f1, f2;
	P x1, x2;

	template<Function<P, V> F>
	BoundsWithValues<P, V> operator()(F& func, BoundsWithValues<P, V> const& r)
	{
		assert(r.l.p < r.r.p);
		auto a = r.l.p, b = r.r.p;

		if (isFirst) {
			x1 = a + (1 - tau) * (b - a),
			x2 = a + tau * (b - a);
			f1 = func(x1);
			f2 = func(x2);
			isFirst = false;
		}

		if (f1 < f2) {
			b = x2;
			x2 = x1;
			f2 = f1;
			x1 = a + (1 - tau) * (b - a);
			f1 = func(x1);
		} else {
			a = x1;
			x1 = x2;
			f1 = f2;
			x2 = a + tau * (b - a);
			f2 = func(x2);
		}

		return {{a, V(0)}, {b, V(0)}};
	}
};
