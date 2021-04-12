#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"

template<typename From, typename To>
class DichotomyApproximator : public BaseApproximator<From, To, DichotomyApproximator<From, To>>
{
	using BaseT = BaseApproximator<From, To, DichotomyApproximator>;

private:
public:
	using IterationData = typename BaseT::IterationData;

	static char const* name() noexcept { return "dichotomy"; }

	using P = From;
	using V = To;

	P epsilon;

	DichotomyApproximator(P epsilon)
	: epsilon(std::move(epsilon))
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r)
	{
		BEGIN_APPROX_COROUTINE(data, r);

		while (r.r > epsilon)
		{
			auto x = r.p;
			auto nl = x - epsilon / 2;
			auto nr = x + epsilon / 2;
			auto lv = func(nl);
			auto rv = func(nr);
			if (lv < rv)
				co_yield r = {r.p - r.r, nr, bound_tag};
			else
				co_yield r = {nl, r.p + r.r, bound_tag};
		}
	}
};
