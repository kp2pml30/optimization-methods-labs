#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"
#include "opt-methods/math/BisquareFunction.hpp"
#include "opt-methods/solvers/function/ErasedFunction.hpp"

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

	Scalar<P> epsilon2;

	ConjugateGradientDescent(decltype(epsilon2) epsilon)
	: epsilon2(epsilon * epsilon)
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func_, PointRegion<P> r)
	{
		BEGIN_APPROX_COROUTINE(data);

		QuadraticFunction<Scalar<P>> *func_ptr;
		if constexpr (std::is_same_v<F, ErasedFunction<V(P const&)>>)
		{
			func_ptr = func_.target<QuadraticFunction2d<Scalar<P>>>();
			if (func_ptr == nullptr)
				func_ptr = func_.target<QuadraticFunction<Scalar<P>>>();
			assert(func_ptr);
		}
		else if constexpr (std::is_same_v<F, QuadraticFunction2d<Scalar<P>>> ||
											 std::is_same_v<F, QuadraticFunction<Scalar<P>>>)
			func_ptr = &func_;
		else
			abort(); // no static assert allowed, thanks GCC UwU
		QuadraticFunction<Scalar<P>> &func = *func_ptr;

		auto gradf = func.grad();

		P x = r.p;
		auto gradx = gradf(x);
		P p = -gradx;
		const auto &A = func.A;

		while (true)
		{
			if (len2(gradx) < epsilon2)
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

