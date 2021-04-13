#pragma once

#include <string>
#include <sstream>
#include <cmath>

#include "./def.hpp"
#include "./Vector.hpp"
#include "./Matrix.hpp"

template<typename T>
class BisquareFunction
{
public:
	Matrix<T> A;
	Vector<T> b;
	T c;

	BisquareFunction(T x2, T xy, T y2, T x, T y, T c)
	: A({2 * x2, xy, xy, 2 * y2})
	, b({x, y})
	, c(c)
	{}

	BisquareFunction(Matrix<T> A, Vector<T> b, T c)
		: A(std::move(A))
		, b(std::move(b))
		, c(std::move(c))
	{
		assert(A.n == b.size());
	}

	BisquareFunction swap() const noexcept
	{
		/// TEST ME
		Vector<T> b_rev = b;
		std::reverse(std::begin(b_rev), std::end(b_rev));
		return BisquareFunction(A.transpose(), b_rev, c);
	}

	T operator()(Vector<T> const& v) const
	{
		assert(v.size() == A.n);
		return dot(A * v, v) / 2 + dot(b, v) + c;
	}

	Vector<T> grad(Vector<T> const& v) const
	{
		assert(v.size() == A.n);
		return A * v + b;
	}

	std::tuple<T, T, T, T, T> get2d_coefs() const
	{
		assert(A.n == 2);
		return {A.data[0], A.data[1] + A.data[2], A.data[3], b[0], b[1]};
	}

	// y2 + (y + xy) + x2 + x + c
	T yDescrSquare(T X) const
	{
		auto [x2, xy, y2, x, y] = get2d_coefs();
		return square(y + xy * X) - 4 * y2 * (x2 * X * X + x * X + c);
	}

	// UI. assumes floating type and NaN evaluation
	T evalYPls(T X) const
	{
		auto [x2, xy, y2, x, y] = get2d_coefs();
		using std::sqrt;
		return (-(y + xy * X) + sqrt(yDescrSquare(X))) / (y2 * 2);
	}
	// UI. assumes floating type and NaN evaluation
	T evalYNeg(T X) const
	{
		auto [x2, xy, y2, x, y] = get2d_coefs();
		using std::sqrt;
		return (-(y + xy * X) - sqrt(yDescrSquare(X))) / (y2 * 2);
	}
	std::pair<T, T> zeroDescrYAt() const
	{
		auto [x2, xy, y2, x, y] = get2d_coefs();
		return solveSquare(xy * xy - 4 * y2 * x2, 2 * y * xy - 4 * y2 * x, y * y - 4 * y2 * c);
	}
	explicit operator std::string() const noexcept
	{
		// return std::format("{}y^2 + {}xy + {}x^2 + {}y + {}x + {} = 0", y2, xy, x2, y, x, c);
		auto [x2, xy, y2, x, y] = get2d_coefs();
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
