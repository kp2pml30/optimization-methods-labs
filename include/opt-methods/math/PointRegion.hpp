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

private:
	static S my_abs(V v)
	{
		using namespace std;
		return abs(v);
	}

public:
	V p;
	S r;

	PointRegion(V p, S r)
	: p(std::move(p))
	, r(std::move(r))
	{}

	PointRegion(V l, V r, bound_tag_t)
	: p((l + r) / 2)
	, r(my_abs(r - l) / 2)
	{}
};
