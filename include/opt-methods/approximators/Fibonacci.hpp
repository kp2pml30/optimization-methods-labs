#pragma once

#include <numbers>
#include <cassert>

#include "opt-methods/solvers/BaseApproximator.hpp"

template<std::floating_point From, typename To, typename FibT = std::size_t>
class FibonacciApproximator : public BaseApproximator<From, To, FibonacciApproximator<From, To, FibT>>
{
	using BaseT = BaseApproximator<From, To, FibonacciApproximator>;

private:
public:
	using IterationData = typename BaseT::IterationData;
	static char const* name() noexcept { return "fibonacci"; }

	using P = From;
	using V = To;

	P l;

	std::tuple<FibT, FibT, int> countFibFromBounds(PointRegion<P> const& r) const
	{
		int n = 1;
		FibT FLast = 1, FPrelast = 0;
		P goal = 2 * r.r / l;
		for (; FLast <= goal; n++)
		{
			auto cur = FLast + FPrelast;
			FPrelast = FLast;
			FLast = cur;
		}

		return {FLast, FPrelast, n};
	}

	P moveBackFib(FibT &FLast, FibT &FPrelast) const
	{
		FibT prev = FLast - FPrelast;
		FLast = FPrelast;
		FPrelast = prev;
		return FPrelast * P(1) / FLast;
	}

	FibonacciApproximator(P l) : l(l) {}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r) const
	{
		BEGIN_APPROX_COROUTINE(data);

		auto [Fnk, Fnk1, n] = countFibFromBounds(r);
		auto [a, fa, b, fb] = this->countBwV(func, r);
		auto tau = Fnk1 * 1.0 / Fnk;
		auto x1 = std::lerp(a, b, 1 - tau), x2 = std::lerp(a, b, tau);
		auto f1 = func(x1), f2 = func(x2);

		for (int k = 1; k < n; k++)
		{
			auto tau = moveBackFib(Fnk, Fnk1);

			if (f1 < f2)
			{
				b = x2, fb = f2;
				x2 = x1, f2 = f1;
				x1 = std::lerp(a, b, 1 - tau), f1 = func(x1);
			}
			else
			{
				a = x1, fa = f1;
				x1 = x2, f1 = f2;
				x2 = std::lerp(a, b, tau), f2 = func(x2);
			}

			co_yield {a, b, bound_tag};
		}
	}
};
