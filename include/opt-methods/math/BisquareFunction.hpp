#pragma once

#include <string>
#include <sstream>
#include <cmath>

#include "./def.hpp"
#include "./Vector.hpp"
#include "./Matrix.hpp"

template<typename T, template<typename> class MatrixImpl = DenseMatrix>
class QuadraticFunction
{
public:
	const MatrixImpl<T> A;
	const Vector<T> b;
	const T c;

	QuadraticFunction(MatrixImpl<T> A, Vector<T> b, T c)
	: A(std::move(A))
	, b(std::move(b))
	, c(std::move(c))
	{
		assert(this->A.dims() == this->b.size());
	}

	QuadraticFunction swap() const noexcept
	{
		/// TEST ME
		Vector<T> b_rev = b;
		std::reverse(std::begin(b_rev), std::end(b_rev));
		return QuadraticFunction(A.transpose(), b_rev, c);
	}

	T operator()(Vector<T> const& v) const
	{
		assert(v.size() == A.dims());
		return dot(A * v, v) / 2 + dot(b, v) + c;
	}

public:
	class GradientFunc
	{
	private:
		friend QuadraticFunction;

		MatrixImpl<T> A;
		Vector<T> b;
		GradientFunc(MatrixImpl<T> A, Vector<T> b)
		: A(std::move(A))
		, b(std::move(b))
		{}
	public:
		Vector<T> operator()(Vector<T> const& v) const
		{
			assert(v.size() == A.dims());
			return A * v + b;
		}
	};

	GradientFunc grad() const
	{
		return GradientFunc(A, b);
	}
};

template<typename T>
class QuadraticFunction2d : public QuadraticFunction<T>
{
public:
	QuadraticFunction2d(T x2, T xy, T y2, T x, T y, T c)
	: QuadraticFunction<T>(DenseMatrix<T>{{x2 * 2, xy, xy, y2 * 2}}, Vector<T>{x, y}, c)
	{}

	std::tuple<T, T, T, T, T> get2d_coefs() const
	{
		auto& A = this->A;
		auto& b = this->b;
		return {A.data[0] / 2, (A.data[1] + A.data[2]) / 2, A.data[3] / 2, b[0], b[1]};
	}

	QuadraticFunction2d shift(T delta_c) const
	{
		auto [x2, xy, y2, x, y] = get2d_coefs();
		return QuadraticFunction2d(x2, xy, y2, x, y, this->c + delta_c);
	}

	// y2 + (y + xy) + x2 + x + c
	T yDescrSquare(T X) const
	{
		auto [x2, xy, y2, x, y] = get2d_coefs();
		return square(y + xy * X) - 4 * y2 * (x2 * X * X + x * X + this->c);
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
		return solveSquare(xy * xy - 4 * y2 * x2, 2 * y * xy - 4 * y2 * x, y * y - 4 * y2 * this->c);
	}
	explicit operator std::string() const noexcept
	{
		// return std::format("{}y^2 + {}xy + {}x^2 + {}y + {}x + {} = 0", y2, xy, x2, y, x, c);
		auto [x2, xy, y2, x, y] = get2d_coefs();
		std::stringstream sstream;
		sstream << y2 << "y^2 + " << xy << "xy + " << x2 << "x^2 + " << y << "y +" << x << "x + " << this->c << " = 0";
		return sstream.str();
	}

	template<typename Random>
	static QuadraticFunction2d Rand(Random const& r)
	{
		return QuadraticFunction2d(r(), r(), r(), r(), r(), r());
	}
};

namespace helper
{
	template<typename T, template<typename> class MatrixImpl>
	MatrixImpl<T> quadraticFunctionMatrixGetter(QuadraticFunction<T, MatrixImpl>);

	template<typename T, typename S, typename U = void>
	struct BaseQuadraticFunctionMatrixParameterT {
	};

	template<typename T, typename S>
	struct BaseQuadraticFunctionMatrixParameterT<T, S, std::void_t<decltype(quadraticFunctionMatrixGetter<S>(std::declval<T>()))>> {
		using type = decltype(quadraticFunctionMatrixGetter<S>(std::declval<T>()));
	};
}

template<typename T, typename S>
concept QuadraticFunctionImpl = requires {
	typename helper::BaseQuadraticFunctionMatrixParameterT<T, S>::type;
};

