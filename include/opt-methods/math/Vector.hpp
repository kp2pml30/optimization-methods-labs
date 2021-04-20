#pragma once

#include <cassert>
#include <valarray>
#include <cmath>

template<typename T>
using Vector = std::valarray<T>;

template<typename T>
T dot(Vector<T> const& l, Vector<T> const& r)
{
	assert(l.size() == r.size());
	return (l * r).sum();
}

template<typename T>
T len2(Vector<T> const& l)
{
	return dot(l, l);
}

template<typename T, typename R = T>
R len(Vector<T> const& l)
{
	return std::sqrt(static_cast<R>(len2(l)));
}
