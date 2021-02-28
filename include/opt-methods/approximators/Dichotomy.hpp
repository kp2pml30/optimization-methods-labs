#pragma once

#include "opt-methods/solvers/Approximator.hpp"

template<typename From, typename To>
class DichotomyApproximator
{
private:
public:
	static char const* name() noexcept { return "dichotomy"; }

	using P = From;
	using V = To;

	P epsilon;

	DichotomyApproximator(P epsilon)
	: epsilon(std::move(epsilon))
	{}

	template<Function<P, V> F>
	Generator<BoundsWithValues<P, V>> operator()(F func, BoundsWithValues<P, V> r)
	{
		assert(r.l.p < r.r.p);

		while (true)
		{
			if (r.r.p - r.l.p < epsilon)
			{
				co_yield r;
				break;
			}
			auto x = (r.l.p + r.r.p) / 2;
			auto nl = x - epsilon;
			auto nr = x + epsilon;
			auto lv = func(nl);
			auto rv = func(nr);
			if (lv < rv)
				co_yield r = {r.l, {nr, rv}};
			else
				co_yield r = {{nl, lv}, r.r};
		}
	}
};

