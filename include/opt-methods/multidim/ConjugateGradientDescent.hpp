#pragma once

#include <concepts>

#include "opt-methods/solvers/BaseApproximator.hpp"
#include "opt-methods/math/BisquareFunction.hpp"
#include "opt-methods/solvers/function/ErasedFunction.hpp"

namespace helper
{
	template<typename T, typename... FuncTs>
	concept FunctionAction = (std::invocable<T, FuncTs const&> && ...);
}

template<typename From, typename To, QuadraticFunctionImpl<Scalar<From>>... FuncTs>
class ConjugateGradientDescent : public BaseApproximator<From, To, ConjugateGradientDescent<From, To>>
{
private:
	using BaseT = BaseApproximator<From, To, ConjugateGradientDescent>;

public:
	using IterationData = typename BaseT::IterationData;

	static char const* name() noexcept { return "conjugate gradient descent"; }

	using P = From;
	using V = To;

private:
	template<QuadraticFunctionImpl<Scalar<From>> FuncHeadT,
					 QuadraticFunctionImpl<Scalar<From>>... FuncTailT>
	void getErasedTarget(ErasedFunction<V(P const &)> const& f, helper::FunctionAction<FuncTs...> auto action)
	{
		auto func = f.template target<FuncHeadT>();
		if (func != nullptr)
			action(*func);
		else if constexpr (sizeof...(FuncTailT) > 0)
			getErasedTarget<FuncTailT...>(f, std::move(action));
		else
			abort(); // nothing matches
	}

public:
	Scalar<P> epsilon2;

	ConjugateGradientDescent(decltype(epsilon2) epsilon)
	: epsilon2(epsilon * epsilon)
	{}

	ApproxGenerator<P, V> implementQuadratic(QuadraticFunctionImpl<Scalar<P>> auto const& func, PointRegion<P> r) {
		BEGIN_APPROX_COROUTINE(data);

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
			decltype(gradx) next_gradx = gradx + alpha * ap;

			Scalar<P> beta = len2(next_gradx) / len2(gradx);
			gradx = next_gradx;
			p = -gradx + beta * p;

			co_yield {x, 0};
		}
	}

	ApproxGenerator<P, V> implementQuadratic(ErasedFunction<V(P const&)> const& func, PointRegion<P> r)
	{
		if constexpr (sizeof...(FuncTs) > 0)
		{
			ApproxGenerator<P, V> res;
			getErasedTarget<FuncTs...>(func, [&, r](auto& f) { res = implementQuadratic(f, r); });
			return res;
		}
		// cannot get anything from erased function
		abort();
	}

	template<Function<P, V> F>
		requires QuadraticFunctionImpl<F, Scalar<P>> || std::convertible_to<F, ErasedFunction<V(P const&)>>
	ApproxGenerator<P, V> operator()(F func_, PointRegion<P> r)
	{
		BEGIN_APPROX_COROUTINE(data);

		auto gen = implementQuadratic(func_, r);

		while (gen.next()) {
			*data = gen.getIterationData();
			co_yield gen.getValue();
		}
	}
};
