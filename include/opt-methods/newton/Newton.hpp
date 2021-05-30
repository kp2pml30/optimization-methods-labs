#pragma once

#include "./NewtonBase.hpp"

struct Void {};

namespace impl
{
	template<typename From, typename To, typename F, typename FGrad, typename FHes>
	struct DefaultNewtonTraits
	{
		struct NewtonState : NewtonStateBase<From, To, F, FGrad, FHes>
		{
			Scalar<From> epsilon2;

			void Initialize(Scalar<From> eps) noexcept
			{
				this->epsilon2 = eps * eps;
				AdvanceP();
			}

			void AdvanceP()
			{
				this->p = this->hess(this->x).Inverse() * this->grad(this->x);
			}

			void FindAlpha()
			{
				while (this->func(this->x) < this->func(this->x - this->p + this->alpha))
					this->alpha /= 2;
			}

			bool Quits()
			{
				return Len2(this->p) * this->alpha * this->alpha <= epsilon2;
			}
		};
	};

	constexpr inline char DefaultNewtonTraitsName[] = "newton";
}

template<typename From, typename To>
using Newton = BaseNewton<From, To, impl::DefaultNewtonTraits, Scalar<From>, impl::DefaultNewtonTraitsName>;
