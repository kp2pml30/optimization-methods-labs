#pragma once

#include "./NewtonBase.hpp"

namespace impl
{
	template<typename From, typename To, Approximator<To, To> OneDimApprox, typename FDec>
	struct NewtonOnedimTraits
	{
		struct NewtonState : NewtonStateBase<From, To, FDec>
		{
			Scalar<From> epsilon2;
			OneDimApprox const* approx;
			bool quits;

			void Initialize(std::tuple<Scalar<From>, OneDimApprox> const& init) noexcept
			{
				quits = false;
				this->epsilon2 = std::get<0>(init);
				this->approx = &std::get<1>(init);
				this->AdvanceP();
			}

			void FindAlpha()
			{
				auto curfunc = [this](double a) {
					return this->func(this->x - a * this->p);
				};
				auto gen = (*approx)(curfunc, {0, this->findRange, bound_tag});
				while (gen.next())
					;
				if (!gen.hasValue())
					quits = true;
				this->alpha = gen.getValue().p;
			}

			bool Quits()
			{
				return quits || Len2(this->p) * this->alpha * this->alpha <= epsilon2;
			}
		};
	};

	constexpr inline char NewtonOnedimTraitsName[] = "newton with minimization";

	template<typename Appr>
	struct NewtonOnedimTraitsBound
	{
		template<typename From, typename To, typename FDec>
		using type = NewtonOnedimTraits<From, To, Appr, FDec>;
	};
}

template<typename From, typename To, Approximator<To, To> OneDimApprox>
using NewtonOnedim = BaseNewton<From, To, impl::NewtonOnedimTraitsBound<OneDimApprox>::template type, std::tuple<Scalar<From>, OneDimApprox>, impl::NewtonOnedimTraitsName>;
