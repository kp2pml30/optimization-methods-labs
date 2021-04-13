#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"
#include "opt-methods/math/BisquareFunction.hpp"

template<typename From, typename To>
class ConjugateGradientDescent : public BaseApproximator<From, To, ConjugateGradientDescent<From, To>>
{
private:
	using BaseT = BaseApproximator<From, To, ConjugateGradientDescent>;

public:
	using IterationData = typename BaseT::IterationData;

	static char const* name() noexcept { return "conjugate gradient descent"; }

	using P = From;
	using V = To;

	Scalar<P> epsilon;

	SteepestDescent(decltype(epsilon) epsilon)
	, epsilon(std::move(epsilon))
	{}

	template<Function<P, V> F>
	requires std::is_same_v<F, BisquareFunction<Scalar<P>>>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r)
	{
		BEGIN_APPROX_COROUTINE(data);

		auto grad = func.gradient();

		P x = r.p;
		auto gradx = grad(x);
		P p = -gradx;
		const auto &A = func.A;

		while (true)
		{
			if (len(gradx) < epsilon)
				break;

			/*
			auto app = dot(A * p, p);
			alpha = -dot(A * gradx, p) / app;
			x += alpha * p;
			gradx = grad(x);
			beta = dot(A * gradx, p) / app;
			p = -gradx + beta * p;
			*/

			auto ap = A * p;
			auto app = dot(ap, p);
			Scalar<P> alpha = len2(gradx) / app;

			x += alpha * p;
			auto next_gradx = gradx + alpha * ap;

			Scalar<P> beta = len2(next_gradx) / len2(gradx);
			gradx = next_gradx;
			p = -gradx + beta * p;

			co_yield {x, 0};
		}
	}
};

