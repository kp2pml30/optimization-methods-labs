#pragma once

#include <memory>
#include <type_traits>

#include "./Approximator.hpp"

namespace QtCharts
{
	class QChart;
}

/**
 * @param CRTP_Child - CRTP class for compile time polymorphism
 */
template<typename From, typename To, typename CRTP_Child>
class BaseApproximator
{
private:
public:
	using P = From;
	using V = To;
	using IterationData = BaseIterationData<P, V>;

	/*ApproximatorImpl<P, V>*/ auto& impl()
	{
		return static_cast<CRTP_Child&>(*this);
	}

	template<Function<P, V> F>
	std::tuple<P, V, P, V> countBwV(F func, PointRegion<P> pr)
	{
		auto l = pr.p - pr.r, r = pr.p + pr.r;
		return {l, func(l), r, func(r)};
	}

	void draw(BoundsWithValues<P, V> r, [[maybe_unused]] IterationData const& data, QtCharts::QChart& chart);

	// template so that can postpone getting IterationData from CRTP child
	auto preproc()
	{
		return std::make_unique<typename CRTP_Child::IterationData>();
	}
};

#define BEGIN_APPROX_COROUTINE(data)						\
	[[maybe_unused]] IterationData *data;					\
	{																							\
		std::unique_ptr u_data = this->preproc();		\
		data = u_data.get();												\
		co_yield std::move(u_data);									\
	}
