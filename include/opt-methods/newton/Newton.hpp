#pragma once

#include "./NewtonBase.hpp"

namespace impl
{
	template<typename From, typename To, typename FDec>
	struct DefaultNewtonTraits
	{
		struct NewtonState : NewtonStateBase<From, To, FDec>
		{
			Scalar<From> epsilon2;

			void Initialize(Scalar<From> eps) noexcept
			{
				this->epsilon2 = eps * eps;
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
