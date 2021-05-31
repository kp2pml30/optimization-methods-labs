#pragma once

#include "./NewtonBase.hpp"

namespace impl
{
	template<typename From, typename To, typename FDec>
	struct NewtonDirectionTraits
	{
		struct NewtonState : NewtonStateBase<From, To, FDec>
		{
			Scalar<From> epsilon2;
			From p0;

			void AdvanceP()
			{
				auto g = this->grad(this->x);
				if (Dot(p0, g) < 0)
					this->p = g;
				else
					this->p = p0;
			}

			void Initialize(std::tuple<Scalar<From>, From> epsDir) noexcept
			{
				std::tie(epsilon2, p0) = std::move(epsDir);
				epsilon2 *= epsilon2;
				AdvanceP();
			}

			bool Quits()
			{
				return Len2(this->p) * this->alpha * this->alpha <= epsilon2;
			}
		};
	};

	constexpr inline char NewtonDirectionTraitsName[] = "newton with direction";
}

template<typename From, typename To>
using NewtonDirection = BaseNewton<From, To, impl::NewtonDirectionTraits, std::tuple<Scalar<From>, From>, impl::NewtonDirectionTraitsName>;
