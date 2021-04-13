#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"

template<typename From, typename To>
class GradientDescent : public BaseApproximator<From, To, GradientDescent<From, To>>
{
private:
	using BaseT = BaseApproximator<From, To, GradientDescent>;

public:
	using IterationData = typename BaseT::IterationData;

	static char const* name() noexcept { return "gradient descent"; }

	using P = From;
	using V = To;

	Scalar<P> epsilon;

	GradientDescent(decltype(epsilon) epsilon)
	: epsilon(std::move(epsilon))
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r)
	{
		BEGIN_APPROX_COROUTINE(data, r);

		/*
		P x = r.p;

		while (true)
		{
			auto grad = func.grad(x);
			auto curfunc = [=](Scalar<P> const& lambda) {
				return func(x - lambda * grad);
			};
			auto gen = onedim(curfunc, {0, 100, bound_tag});
			while (gen.next())
				;
			auto lambda = gen.getValue().p;
			auto delta = lambda * grad;
			x -= delta;
			co_yield {x, 0};
			if (len(static_cast<P const&>(delta)) < epsilon)
				break;
		}
		*/

		P x = r.p;
		auto fx = func(x);
		Scalar<P> alpha = 0.1;

		while (true)
		{
			auto grad = func.grad(x);
			if (len(grad) < epsilon)
				break;
			P y;
			V fy;
			do
			{
				y = x - alpha * grad;
				fy = func(y);
				alpha /= 2;
			} while (fy >= fx);
			alpha *= 2;
			x = y;
			fx = fy;
			co_yield {x, 0};
		}
	}
};

