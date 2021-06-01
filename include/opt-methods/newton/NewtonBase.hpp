#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"
#include <iostream>

template<typename Ff, typename Gg, typename Hh>
struct DecomposedFuncTypes
{
	using F = Ff;
	using G = Gg;
	using H = Hh;
};

template<typename From, typename To, typename F>
struct NewtonStateBase;

template<typename From, typename To, typename F, typename FGrad, typename FHes>
struct NewtonStateBase<From, To, DecomposedFuncTypes<F, FGrad, FHes>>
{
	From x;
	From p;
	Scalar<From> alpha;
	F func;
	FGrad grad;
	FHes hess;
	Scalar<From> findRange;

	/// use shadowing to override
	void AdvanceP()
	{
		this->p = this->hess(this->x).Inverse() * this->grad(this->x);
	}

	/// use shadowing to override
	void FindAlpha()
	{
		while (this->func(this->x) < this->func(this->x - this->p * this->alpha))
			this->alpha /= 2;
	}
};

template<typename This, typename From, typename To, typename FDec, typename Initializer>
concept NewtonStateTraits = requires(Initializer const& init) {
	typename This::NewtonState;
	std::derived_from<typename This::NewtonState, NewtonStateBase<From, To, FDec>>;
	requires requires(typename This::NewtonState& st) {
		{ st.Initialize(init) };
		{ st.AdvanceP() };
		{ st.FindAlpha() };
		{ st.Quits() } -> std::same_as<bool>;
	};
};

namespace impl
{
	auto DecomposeFuncTypes(auto func)
	{
		return DecomposedFuncTypes<std::decay_t<decltype(func)>,
				std::decay_t<decltype(func.grad())>,
				std::decay_t<decltype(func.hessian())>>{};
	}
}

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

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r) const
		requires NewtonStateTraits<traits<From, To, decltype(impl::DecomposeFuncTypes(func))>, From, To, decltype(impl::DecomposeFuncTypes(func)), Initializer>
	{
		BEGIN_APPROX_COROUTINE(data);

		typename traits<From, To, decltype(impl::DecomposeFuncTypes(func))>::NewtonState
			state{r.p, {}, r.r, func, func.grad(), func.hessian(), r.r};

		state.Initialize(static_cast<Initializer const&>(initializer)); // ensure not changed

		do
		{
			state.AdvanceP();
			state.FindAlpha();
			state.x -= state.p * state.alpha;

			co_yield {state.x, 0};
		} while (!state.Quits());
	}
};

