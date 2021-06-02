#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"

template<typename From, typename To>
class Marquardt1 : public BaseApproximator<From, To, Marquardt1<From, To>>
{
private:
	using BaseT = BaseApproximator<From, To, Marquardt1>;

public:
	using P = From;
	using V = To;
	using S = Scalar<P>;

	struct IterationData : BaseT::IterationData
	{
		S tau;
	};

	static char const* name() noexcept { return "marquardt 1"; }

	S epsilon2;
	S tau0, beta;

	Marquardt1(S epsilon, S tau0, S beta)
	: epsilon2(epsilon * epsilon)
	, tau0(tau0)
	, beta(beta)
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r) const
	{
		BEGIN_APPROX_COROUTINE(data);

		auto I = DenseMatrix<S>::Identity(r.p.size());

		P x = r.p, p;
		V fx;
		auto gradf = func.grad();
		auto hessf = func.hessian();

		S tau0 = this->tau0, tau;

		while (true)
		{
			tau = tau0;
			fx = func(x);
			auto grad = gradf(x);
			auto hess = hessf(x);

			P y;
			V fy;
			while (true)
			{
				p = DenseMatrix<S>(hess + I * tau).SolveSystem(-grad);
				y = x + p, fy = func(y);
				if (fy > fx)
					tau /= beta;
				else
					break;
			}
			x = y, fx = fy, tau0 *= beta;

			if (Len2(p) < epsilon2) break;
			data->tau = tau;
			co_yield {x, 0};
		}
	}
};
