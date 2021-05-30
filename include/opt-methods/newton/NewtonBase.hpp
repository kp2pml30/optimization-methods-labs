#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"
#include <iostream>

template<typename From, typename To, typename F, typename FGrad, typename FHes>
struct NewtonStateBase
{
	From x;
	From p;
	Scalar<From> alpha;
	F func;
	FGrad grad;
	FHes hess;
};

template<typename This, typename From, typename To, typename F, typename FGrad, typename FHes, typename Initializer>
concept NewtonStateTraits = requires(Initializer const& init) {
	typename This::NewtonState;
	std::derived_from<typename This::NewtonState, NewtonStateBase<From, To, F, FGrad, FHes>>;
	requires requires(typename This::NewtonState& st) {
		{ st.Initialize(init) };
		{ st.AdvanceP() };
		{ st.FindAlpha() };
		{ st.Quits() } -> std::same_as<bool>;
	};
};

template<typename From, typename To, template<typename...> typename traits, typename Initializer, char const* nname>
class BaseNewton : public BaseApproximator<From, To, BaseNewton<From, To, traits, Initializer, nname>>
{
private:
	using BaseT = BaseApproximator<From, To, BaseNewton>;
	Initializer initializer;

public:
	using IterationData = typename BaseT::IterationData;

	static char const* name() noexcept { return nname; }

	using P = From;
	using V = To;

	BaseNewton(Initializer initializer)
	: initializer(std::move(initializer))
	{}

	ApproxGenerator<P, V> operator()(Function<P, V> auto func, PointRegion<P> r)
		requires NewtonStateTraits<traits<From, To, decltype(func), decltype(func.grad()), decltype(func.hessian())>, From, To, decltype(func), decltype(func.grad()), decltype(func.hessian()), Initializer>
	{
		BEGIN_APPROX_COROUTINE(data);

		typename traits<From, To, decltype(func), decltype(func.grad()), decltype(func.hessian())>::NewtonState
			state{r.p, {}, r.r, func, func.grad(), func.hessian()};

		state.Initialize(static_cast<Initializer const&>(initializer)); // ensure not changed

		while (!state.Quits())
		{
			state.AdvanceP();
			state.FindAlpha();
#if 0
			std::cout << "p :";
			for (std::size_t i = 0; i < state.p.size(); i++)
				std::cout << ' '<< state.p[i];
			std::cout << std::endl << "alpha :" << state.alpha << std::endl;
#endif
			state.x -= state.p * state.alpha;

			co_yield {state.x, 0};
		}
	}
};

