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
	ApproxGenerator<P, V> operator()(F func, BoundsWithValues<P, V> r)
	{
		IterationData* data;
		co_yield data = this->preproc(r);

		while ((r.r.p - r.l.p) / 2 > epsilon)
		{
			auto x = (r.l.p + r.r.p) / 2;
			auto nl = x - epsilon / 2;
			auto nr = x + epsilon / 2;
			auto lv = func(nl);
			auto rv = func(nr);
			if (lv < rv)
				co_yield r = {r.l, {nr, rv}};
			else
				co_yield r = {{nl, lv}, r.r};
		}
	}
};
