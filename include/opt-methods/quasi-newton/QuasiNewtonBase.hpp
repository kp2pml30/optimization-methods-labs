#pragma once

#include "../newton/NewtonBase.hpp"
#include "../newton/NewtonOnedim.hpp"

namespace impl
{
	template<typename From, typename To, typename Initializer, typename FDec,
	         NewtonStateTraits<From, To, FDec, Initializer> BaseTraits,
	         typename CRTP_Child>
	struct QuasiNewtonTraits
	{
		struct NewtonState : BaseTraits::NewtonState
		{
			using BaseT = BaseTraits::NewtonState;

			Scalar<From> epsilon2;
			DenseMatrix<Scalar<From>> G;
			From curGrad, lastGrad, dGrad;
			From lastX, dx;
			bool isFirst;

			void Initialize(Scalar<From> eps) noexcept
			{
				epsilon2 = eps * eps;

				G = DenseMatrix<Scalar<From>>::Identity(this->x.size());
				lastX = From(0.0, this->x.size());
				curGrad = -this->grad(this->x);
				isFirst = true;
			}

			/// use shadowing to override
			void CalcG()
			{}

			void AdvanceP()
			{
				if (isFirst)
				{
					/* dGrad_{k-1}, G_{k} already calculated */
					isFirst = false;
				}
				else
				{
					/* calculate dGrad_{k-1}, G_{k} */
					// dx_{k-1} is already calculated in Quits
					// dGrad_{k-1}
					lastGrad = curGrad, curGrad = -this->grad(this->x);
					dGrad = curGrad - lastGrad;
					static_cast<CRTP_Child::NewtonState*>(this)->CalcG();
				}

				this->p = -(G * curGrad);  // calculate p_k
			}

			bool Quits()
			{
				dx = this->x - lastX, lastX = this->x;
				return !isFirst && Len2(dx) <= epsilon2;
			}
		};
	};
}
