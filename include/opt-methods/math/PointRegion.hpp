#pragma once

#include "./Scalar.hpp"

template<typename T>
struct PointRegion
{
	using V = T;
	using S = Scalar<V>;

	V p;
	S r;

	PointRegion(V p, S r)
	: p(std::move(p))
	, r(std::move(r))
	{}
};
