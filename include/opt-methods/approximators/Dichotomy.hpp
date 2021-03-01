#pragma once

#include "opt-methods/solvers/Approximator.hpp"

template<typename From, typename To>
class DichotomyApproximtor
{
private:
public:
	static char const* name() noexcept { return "dichotomy"; }

	using P = From;
	using V = To;

	P epsilon;

	DichotomyApproximtor(P epsilon)
	: epsilon(std::move(epsilon))
	{}

	template<Function<P, V> F>
	BoundsWithValues<P, V> operator()(F& func, BoundsWithValues<P, V> const& r)
	{
		assert(r.l.p < r.r.p);
		if (r.r.p - r.l.p < epsilon)
			return r;
		auto x = (r.l.p + r.r.p) / 2;
		auto nl = x - epsilon;
		auto nr = x + epsilon;
		auto lv = func(nl);
		auto rv = func(nr);
		if (lv < rv)
			return {r.l, {nr, rv}};
		else
			return {{nl, lv}, r.r};
	}
};

