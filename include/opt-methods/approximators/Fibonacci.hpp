#pragma once

#include <numbers>
#include <cassert>

#include "opt-methods/solvers/Approximator.hpp"

template<std::floating_point From, typename To, typename FibT>
class FibonacciApproximtor
{
private:
public:
	static char const* name() noexcept { return "fibonacci"; }

	using P = From;
	using V = To;

	P l;

	std::tuple<FibT, FibT, int> countFibFromBounds(BoundsWithValues<P, V> const& r) {
		int n = 1;
		FibT FLast = 1, FPrelast = 0;
		P goal = (r.r.p - r.l.p) / l;
		for (; FLast <= goal; n++) {
			auto cur = FLast + FPrelast;
			FPrelast = FLast;
			FLast = cur;
		}

		return {FLast, FPrelast, n};
	}

	P moveBackFib(FibT &FLast, FibT &FPrelast) {
		FibT prev = FLast - FPrelast;
		FLast = FPrelast;
		FPrelast = prev;
		return FPrelast * P(1) / FLast;
	}

	FibonacciApproximtor(P l) : l(l) {}

	template<Function<P, V> F>
	Generator<BoundsWithValues<P, V>> operator()(F func, BoundsWithValues<P, V> const& r) {
		assert(r.l.p < r.r.p);

		auto [Fnk, Fnk1, n] = countFibFromBounds(r);
		auto a = r.l.p, b = r.r.p;
		auto fa = r.l.v, fb = r.r.v;
		auto tau = Fnk1 * 1.0 / Fnk;
		auto x1 = a + (1 - tau) * (b - a), x2 = a + tau * (b - a);
		auto f1 = func(x1), f2 = func(x2);

		for (int k = 1; k < n; k++) {
			auto tau = moveBackFib(Fnk, Fnk1);

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
