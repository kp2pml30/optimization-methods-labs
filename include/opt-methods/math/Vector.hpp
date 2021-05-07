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
T dot(Vector<T> const& l, Vector<T> const& r)
{
	assert(l.size() == r.size());
	if (l.size() == 0)
		return {};
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

namespace util
{
	template<typename TT>
	std::ostream& operator<<(std::ostream& o, Vector<TT> const& v)
	{
		WriteVector(o, v);
		return o;
	}

	template<typename TT>
	std::istream& operator>>(std::istream& i, Vector<TT> const& v)
	{
		std::vector<TT> data;
		ReadVector(i, data);
		v = std::valarray<TT>(data.data(), data.size());
		return i;
	}
}