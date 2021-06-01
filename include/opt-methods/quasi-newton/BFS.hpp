#pragma once

#include "../newton/NewtonBase.hpp"
#include "../newton/NewtonOnedim.hpp"
#include "./QuasiNewtonBase.hpp"

namespace impl
{
	template<typename From, typename To, Approximator<To, To> OneDimApprox, typename FDec>
	struct BFSNewtonTraits
	{
		using BaseT = impl::QuasiNewtonTraits<From,
		                                      To,
		                                      std::tuple<Scalar<From>, OneDimApprox>,
		                                      FDec,
		                                      NewtonOnedimTraits<From, To, OneDimApprox, FDec>,
		                                      BFSNewtonTraits>::NewtonState;

		struct NewtonState : BaseT
		{
			From r;

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
				// rho_{k-1}
				auto GdGrad = this->G * this->dGrad;
				auto
					dxDGrad = Dot(this->dx, this->dGrad),
					rho = Dot(GdGrad, this->dGrad);
				// r_{k-1}
				r = GdGrad / rho - this->dx / dxDGrad;
				// G_{k}
				using M = DenseMatrix<Scalar<From>>;
				this->G += -M::TensorProduct(this->dx, this->dx) / dxDGrad -
					M::TensorProduct(GdGrad, GdGrad) / rho +
					M::TensorProduct(r, r) * rho;
			}

			bool Quits()
			{
				return this->quits || BaseT::Quits();
			}
		};
	};

	constexpr inline char BFSNewtonTraitsName[] = "broyden fletcher shanno";

	template<typename Appr>
	struct BFSNewtonTraitsBound
	{
		template<typename From, typename To, typename FDec>
		using type = BFSNewtonTraits<From, To, Appr, FDec>;
	};
}

template<typename From, typename To, Approximator<To, To> OneDimApprox>
using QuasiNewtonBFS = BaseNewton<From, To, impl::BFSNewtonTraitsBound<OneDimApprox>::template type,
	                                std::tuple<Scalar<From>, OneDimApprox>, impl::BFSNewtonTraitsName>;
