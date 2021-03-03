#pragma once

#include "Approximator.hpp"
#include "opt-methods/util/Charting.hpp"

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

	void draw(BoundsWithValues<P, V> r, IterationData const& data, QtCharts::QChart& chart)
	{
		using namespace QtCharts;

		Charting::addToChart(&chart, Charting::drawPoints({QPointF{r.l.p, r.l.v}, QPointF{r.r.p, r.r.v}}, "Search bounds"));
		if constexpr (HasDrawImpl<CRTP_Child, P, V>)
			impl().draw_impl(r, static_cast<typename CRTP_Child::IterationData const&>(data), chart);
	}

	template<Function<P, V> F>
	ApproxGenerator<P, V> operator()(F func, BoundsWithValues<P, V> r)
	{
		auto holder = std::make_unique<typename CRTP_Child::IterationData>();
		auto gen = impl().begin_impl(std::move(func), std::move(r), *holder);
		gen.setData(std::move(holder));
		return gen;
	}
};
