#pragma once

#include <cassert>
#include <valarray>
#include <cmath>
#include <iostream>
#include <vector>

#include "../util/Util.hpp"

template<typename T>
using Vector = std::valarray<T>;

template<typename T>
T Dot(Vector<T> const& l, Vector<T> const& r)
{
	assert(l.size() == r.size());
	if (l.size() == 0)
		return {};
	return (l * r).sum();
}

template<typename T>
T Len2(Vector<T> const& l)
{
	return Dot(l, l);
}

template<typename T, typename R = T>
R Len(Vector<T> const& l)
{
	return std::sqrt(static_cast<R>(Len2(l)));
}

template<std::floating_point T, typename R = T>
R Len2(T x)
{
	return static_cast<R>(x * x);
}

template<std::floating_point T, typename R = T>
R Len(T x)
{
	return static_cast<R>(std::abs(x));
}

namespace util
{
	template<typename TT>
	std::ostream& operator<<(std::ostream& o, Vector<TT> const& v)
	{
		WriteVector(o, v);
		return o;
	}

	template<typename TT>
	std::istream& operator>>(std::istream& i, Vector<TT>& v)
	{
		std::vector<TT> data;
		ReadVector(i, data);
		v = std::valarray<TT>(data.data(), data.size());
		return i;
	}
}
