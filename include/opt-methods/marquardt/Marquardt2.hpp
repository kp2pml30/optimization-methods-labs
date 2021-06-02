#pragma once

#include "opt-methods/solvers/BaseApproximator.hpp"
#include "./Marquardt1.hpp"

template<typename From, typename To>
class Marquardt2 : public BaseApproximator<From, To, Marquardt2<From, To>>
{
private:
	using BaseT = BaseApproximator<From, To, Marquardt2>;

public:
	struct IterationData : Marquardt1<From, To>::IterationData
	{
		int nCholesky = 0;
	};

	static char const* name() noexcept { return "marquardt 2"; }

	using P = From;
	using V = To;
	using S = Scalar<P>;

	S epsilon2, beta;

	Marquardt2(S epsilon, S beta)
	: epsilon2(epsilon * epsilon)
	, beta(beta)
	{}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, PointRegion<P> r) const
	{
		BEGIN_APPROX_COROUTINE(data);

		auto I = DenseMatrix<S>::Identity(r.p.size());

		P x = r.p, p;
		auto gradf = func.grad();
		auto hessf = func.hessian();

		S tau = 0;

		while (true)
		{
			auto grad = gradf(x);
			auto hess = hessf(x);

			while (!CholeskySolveSystem(DenseMatrix<S>(hess + I * tau), -grad, p))
			{
				data->nCholesky++;
				using std::max;
				tau = max<S>(1, 2 * tau);
			}
			data->tau = tau;
			data->nCholesky++;
			x += p;

			if (Len2(p) < epsilon2) break;
			co_yield {x, 0};
			tau *= beta;
		}
	}
};
