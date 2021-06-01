#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"

template<typename From, typename To, Approximator<To, To> OneDimApprox>
class SteepestDescent : public BaseApproximator<From, To, SteepestDescent<From, To, OneDimApprox>>
{
private:
	using BaseT = BaseApproximator<From, To, SteepestDescent>;
	[[no_unique_address]] OneDimApprox onedim;

public:
	using IterationData = typename BaseT::IterationData;

	static char const* name() noexcept { return "steepest descent"; }

	using P = From;
	using V = To;

	Scalar<P> epsilon2;

	SteepestDescent(decltype(epsilon2) epsilon, OneDimApprox onedim)
	: onedim(std::move(onedim))
	, epsilon2(epsilon * epsilon)
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r) const
	{
		BEGIN_APPROX_COROUTINE(data);

		P x = r.p;
		auto gradf = func.grad();

		while (true)
		{
			auto grad = gradf(x);
			if (Len2(grad) < epsilon2) break;

			auto curfunc = [=](Scalar<P> const& lambda) {
				return func(x - lambda * grad);
			};
			auto gen = onedim(curfunc, {0, r.r, bound_tag});
			while (gen.next())
				;
			if (!gen.hasValue())
			{
				co_yield {x, r.r};
				break;
			}

			x -= gen.getValue().p * grad;
			co_yield {x, 0};
		}
	}
};

