#pragma once

#include "../newton/NewtonBase.hpp"
#include "../newton/NewtonOnedim.hpp"
#include "./QuasiNewtonBase.hpp"

namespace impl
{
	template<typename From, typename To, Approximator<To, To> OneDimApprox, typename FDec>
	struct PowellNewtonTraits
	{
		using BaseT = impl::QuasiNewtonTraits<From,
		                                      To,
		                                      std::tuple<Scalar<From>, OneDimApprox>,
		                                      FDec,
		                                      NewtonOnedimTraits<From, To, OneDimApprox, FDec>,
		                                      PowellNewtonTraits>::NewtonState;

		struct NewtonState : BaseT
		{
			From dxTilde;

			void Initialize(std::tuple<Scalar<From>, OneDimApprox> const& init) noexcept
			{
				BaseT::Initialize(std::get<0>(init));
				this->quits = false;
				this->epsilon2 = std::get<0>(init);
				this->approx = &std::get<1>(init);
			}

			void CalcG()
			{
				// pre: dx_{k-1}, dGrad_{k-1} already calculated
				/* calculate G_k */
				// dx~_{k-1}
				dxTilde = this->dx + this->G * this->dGrad;
				// G_{k}
				using M = DenseMatrix<Scalar<From>>;
				this->G -= M::TensorProduct(dxTilde, dxTilde) / Dot(this->dGrad, dxTilde);
			}

			bool Quits()
			{
				return this->quits || BaseT::Quits();
			}
		};
	};

	constexpr inline char PowellNewtonTraitsName[] = "powell";

	template<typename Appr>
	struct PowellNewtonTraitsBound
	{
		template<typename From, typename To, typename FDec>
		using type = PowellNewtonTraits<From, To, Appr, FDec>;
	};
}

template<typename From, typename To, Approximator<To, To> OneDimApprox>
using QuasiNewtonPowell = BaseNewton<From, To, impl::PowellNewtonTraitsBound<OneDimApprox>::template type,
	                                   std::tuple<Scalar<From>, OneDimApprox>, impl::PowellNewtonTraitsName>;
