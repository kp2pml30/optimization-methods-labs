#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"
#include <iostream>
template<typename From, typename To, Approximator<To, To> OneDimApprox>
class GradientDescent : public BaseApproximator<From, To, GradientDescent<From, To, OneDimApprox>>
{
private:
	using BaseT = BaseApproximator<From, To, GradientDescent>;

public:
	using IterationData = typename BaseT::IterationData;

	static char const* name() noexcept { return "gradient descent"; }

	using P = From;
	using V = To;

	V epsilon;

	GradientDescent(V epsilon)
	: epsilon(std::move(epsilon))
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, BoundsWithValues<P, V> r)
	{
		IterationData* data;
		co_yield data = this->preproc(r);

		OneDimApprox onedim{epsilon};

		P x = r.l.p;

		for (int i = 0; i < 10; i++)
		{
			auto grad = func.grad(x);
			auto curfunc = [=](V const& lambda) {
				return func(x - lambda * grad);
			};
			auto gen = onedim(curfunc, {{0, curfunc(0)}, {100, curfunc(100)}});
			while (gen.next())
				;
			auto lambda = gen.getValue().l.p;
			x -= lambda * grad;
			co_yield {{x, func(x)}, {{}, 0}};
		}
	}
};

