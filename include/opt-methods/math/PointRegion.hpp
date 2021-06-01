#pragma once

#include "./Vector.hpp"
#include "./Scalar.hpp"

inline constexpr struct bound_tag_t
{} bound_tag;

template<typename T>
struct PointRegion
{
	using V = T;
	using S = Scalar<V>;

public:
	V p;
	S r;

	PointRegion(V p, S r)
	: p(std::move(p))
	, r(std::move(r))
	{}

	PointRegion(V l, V r, bound_tag_t)
	: p((l + r) / 2)
	, r(Len(r - l) / 2)
	{}
};
