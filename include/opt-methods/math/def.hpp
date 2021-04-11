#pragma once

#include <cmath>

template<typename T, typename R = T>
R square(T const& a)
{
	auto const& b = static_cast<R>(a);
	return b * b;
}

// returns NaN on error
template<typename T>
std::pair<T, T> solveSquare(T a, T b, T c)
{
	using std::sqrt;
	auto am2 = a * 2;
	auto bneg = -b;
	auto sqr = sqrt(b * b - a * c * 4);
	return {(bneg - sqr) / am2, (bneg + sqr) / am2};
}

