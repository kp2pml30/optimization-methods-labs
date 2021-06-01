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

	Scalar<P> epsilon2;

	GradientDescent(decltype(epsilon2) epsilon)
	: epsilon2(epsilon * epsilon)
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r) const
	{
		BEGIN_APPROX_COROUTINE(data);

		P x = r.p;
		auto fx = func(x);
		Scalar<P> alpha = r.r;

		auto gradf = func.grad();

		while (true)
		{
			auto grad = gradf(x);
			if (Len2(grad) < epsilon2)
				break;
			P y = x;
			V fy = fx;
			while (alpha > 0)
			{
				y = x - alpha * grad;
				fy = func(y);
				if (fy < fx) break;
				alpha /= 2;
			}
			x = y;
			fx = fy;
			co_yield {x, 0};
		}
	}
};

