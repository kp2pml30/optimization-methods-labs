#pragma once

#include "./NewtonOnedim.hpp"

namespace impl
{
	template<typename From, typename To, Approximator<To, To> OneDimApprox, typename FDec>
	struct NewtonDirectionTraits
	{
		struct NewtonState : NewtonOnedimTraits<From, To, OneDimApprox, FDec>::NewtonState
		{
			using BaseT = NewtonOnedimTraits<From, To, OneDimApprox, FDec>::NewtonState;
			bool isFirst;

			void AdvanceP()
			{
				auto g = this->grad(this->x);
				if (isFirst)
				{
					isFirst = false;
					this->p = g;
				}
				else
				{
					BaseT::AdvanceP();  // solve SLE
					if (Dot(this->p, g) < 0)
						this->p = g;  // fall back to antigradient
				}
			}

			void Initialize(std::tuple<Scalar<From>, OneDimApprox>const& epsApp) noexcept
			{
				NewtonOnedimTraits<From, To, OneDimApprox, FDec>::NewtonState::Initialize(epsApp);
				isFirst = true;
			}
		};
	};

	constexpr inline char NewtonDirectionTraitsName[] = "newton with direction";

	template<typename Appr>
	struct NewtonDirectionTraitsBound
	{
		template<typename From, typename To, typename FDec>
		using type = NewtonDirectionTraits<From, To, Appr, FDec>;
	};
}


template<typename From, typename To, Approximator<To, To> OneDimApprox>
using NewtonDirection = BaseNewton<From, To, impl::NewtonDirectionTraitsBound<OneDimApprox>::template type, std::tuple<Scalar<From>, OneDimApprox>, impl::NewtonDirectionTraitsName>;
