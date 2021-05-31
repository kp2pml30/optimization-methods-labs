#pragma once

#include "./NewtonOnedim.hpp"

namespace impl
{
	template<typename From, typename To, Approximator<To, To> OneDimApprox, typename FDec>
	struct NewtonDirectionTraits
	{
		struct NewtonState : NewtonOnedimTraits<From, To, OneDimApprox, FDec>::NewtonState
		{
			From p0;

			void AdvanceP()
			{
				auto g = this->grad(this->x);
				g /= Len(g);
				if (Dot(p0, g) <= 1e-3)
					this->p = g;
				else
					this->p = p0;
			}

			void Initialize(std::tuple<Scalar<From>, From, OneDimApprox>const& epsDirApp) noexcept
			{
				NewtonOnedimTraits<From, To, OneDimApprox, FDec>::NewtonState::Initialize(
						std::get<0>(epsDirApp),
						std::get<2>(epsDirApp));
				p0 = std::get<1>(epsDirApp);
				p0 /= Len(p0);
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
using NewtonDirection = BaseNewton<From, To, impl::NewtonDirectionTraitsBound<OneDimApprox>::template type, std::tuple<Scalar<From>, From, OneDimApprox>, impl::NewtonDirectionTraitsName>;
