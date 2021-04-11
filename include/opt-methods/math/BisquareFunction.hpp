#pragma once

#include <string>
#include <sstream>
#include <cmath>

#include "./def.hpp"

template<typename T>
class BisquareFunction
{
public:
	T x2, xy, y2, x, y, c;

	BisquareFunction(T x2, T xy, T y2, T x, T y, T c)
	: x2(std::move(x2))
	, xy(std::move(xy))
	, y2(std::move(y2))
	, x(std::move(x))
	, y(std::move(y))
	, c(std::move(c))
	{}

	BisquareFunction swap() const noexcept
	{
		return BisquareFunction(y2, xy, x2, y, x, c);
	}

	T operator()(T const& X, T const& Y) const
	{
		return x2 * X * X + xy * X * Y + y2 * Y * Y + x * X + y * Y + c;
	}

	// y2 + (y + xy) + x2 + x + c
	T yDescrSquare(T X) const
	{
		return square(y + xy * X) - 4 * y2 * (x2 * X * X + x * X + c);
	}

	// UI. assumes floating type and NaN evaluation
	T evalYPls(T X) const
	{
		using std::sqrt;
		return (-(y + xy * X) + sqrt(yDescrSquare(X))) / (y2 * 2);
	}
	// UI. assumes floating type and NaN evaluation
	T evalYNeg(T X) const
	{
		using std::sqrt;
		return (-(y + xy * X) - sqrt(yDescrSquare(X))) / (y2 * 2);
	}
	std::pair<T, T> zeroDescrYAt() const
	{
		return solveSquare(xy * xy - 4 * y2 * x2, 2 * y * xy - 4 * y2 * x, y * y - 4 * y2 * c);
	}
	explicit operator std::string() const noexcept
	{
		// return std::format("{}y^2 + {}xy + {}x^2 + {}y + {}x + {} = 0", y2, xy, x2, y, x, c);
		std::stringstream sstream;
		sstream << y2 << "y^2 + " << xy << "xy + " << x2 << "x^2 + " << y << "y +" << x << "x + " << c << " = 0";
		return sstream.str();
	}

	template<typename Random>
	static BisquareFunction Rand(Random const& r)
	{
		return BisquareFunction(r(), r(), r(), r(), r(), r());
	}
};
