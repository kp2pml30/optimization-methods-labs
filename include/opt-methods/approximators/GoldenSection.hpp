#pragma once

#include <numbers>
#include <cassert>

#include "opt-methods/solvers/BaseApproximator.hpp"

template<std::floating_point From, typename To>
class GoldenSectionApproximator : public BaseApproximator<From, To, GoldenSectionApproximator<From, To>>
{
	using BaseT = BaseApproximator<From, To, GoldenSectionApproximator>;

private:
public:
	using IterationData = typename BaseT::IterationData;
	static char const* name() noexcept { return "golden section"; }

	using P = From;
	using V = To;

	static constexpr P tau = std::numbers::phi_v<P> - 1;

	P epsilon;

	GoldenSectionApproximator(P epsilon)
	: epsilon(epsilon)
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r)
	{
		using std::lerp;

		BEGIN_APPROX_COROUTINE(data, r);

		auto [a, fa, b, fb] = this->countBwV(func, r);

		P x1 = lerp(a, b, 1 - tau), x2 = lerp(a, b, tau);
		V f1 = func(x1), f2 = func(x2);

		while (b - a >= epsilon)
		{
			if (f1 < f2)
			{
				b = x2, fb = f2;
				x2 = x1, f2 = f1;
				x1 = lerp(a, b, 1 - tau), f1 = func(x1);
			}
			else
			{
				a = x1, fa = f1;
				x1 = x2, f1 = f2;
				x2 = lerp(a, b, tau), f2 = func(x2);
			}

			co_yield {a, b, bound_tag};
		}
	}
};
