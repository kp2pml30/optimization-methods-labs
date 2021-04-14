#pragma once

#include "opt-methods/util/Charting.hpp"
#include "./BaseApproximator.hpp"

template<typename From, typename To, typename CRTP_Child>

void BaseApproximator<From, To, CRTP_Child>::draw(BoundsWithValues<P, V> r, IterationData const& data, QtCharts::QChart& chart)
{
	using namespace QtCharts;

	if constexpr (std::is_floating_point_v<decltype(r.l.p)>)
		Charting::addToChart(&chart, Charting::drawPoints({QPointF{r.l.p, r.l.v}, QPointF{r.r.p, r.r.v}}, "Search bounds"));
	if constexpr (HasDrawImpl<CRTP_Child, P, V>)
		impl().draw_impl(r, static_cast<typename CRTP_Child::IterationData const&>(data), chart);
}

