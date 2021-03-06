#pragma once

#include <string>
#include <sstream>
#include <cmath>
#include <tuple>

#include "./def.hpp"
#include "./Vector.hpp"
#include "./Matrix.hpp"
#include "./DenseMatrix.hpp"

template<typename T, Matrix<T> MatrixImpl = DenseMatrix<T>>
class QuadraticFunction
{
public:
	const MatrixImpl A;
	const Vector<T> b;
	const T c;

	QuadraticFunction(MatrixImpl A, Vector<T> b, T c)
	: A(std::move(A))
	, b(std::move(b))
	, c(std::move(c))
	{
		assert(this->A.Dims() == this->b.size());
	}

	QuadraticFunction swap() const noexcept
	{
		/// TEST ME
		Vector<T> b_rev = b;
		std::reverse(std::begin(b_rev), std::end(b_rev));
		return QuadraticFunction(A.Transpose(), b_rev, c);
	}

	T operator()(Vector<T> const& v) const
	{
		assert(v.size() == A.Dims());
		return Dot(A * v, v) / 2 + Dot(b, v) + c;
	}

	class HessianFunc
	{
	private:
		MatrixImpl A;
	public:
		HessianFunc(MatrixImpl A)
		: A(std::move(A))
		{}

		MatrixImpl operator()(Vector<T> const&) const noexcept
		{
			return A;
		}
	};
	class GradientFunc
	{
	private:
		friend QuadraticFunction;

		MatrixImpl A;
		Vector<T> b;
		GradientFunc(MatrixImpl A, Vector<T> b)
		: A(std::move(A))
		, b(std::move(b))
		{}
	public:
		Vector<T> operator()(Vector<T> const& v) const
		{
			assert(v.size() == A.Dims());
			return A * v + b;
		}
	};

	GradientFunc grad() const
	{
		return GradientFunc(A, b);
	}
	HessianFunc hessian() const
	{
		return {A};
	}
};

template<typename T, Matrix<T> MatrixImpl>
QuadraticFunction(MatrixImpl, Vector<T>, T) -> QuadraticFunction<T, MatrixImpl>;

template<typename T>
class QuadraticFunction2d : public QuadraticFunction<T>
{
public:
	QuadraticFunction2d(T x2, T xy, T y2, T x, T y, T c)
	: QuadraticFunction<T>(DenseMatrix<T>{2, {x2 * 2, xy, xy, y2 * 2}}, Vector<T>{x, y}, c)
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
		bool was = false;
		auto const& first = [&](auto const& coef, std::string_view str) {
			if (coef == 0)
				return;
			if (std::abs(coef) == 1)
			{
				if (coef < 0)
					sstream << "-";
			}
			else
				sstream << coef;
			was = true;
			sstream << str;
		};
		auto const& next = [&](auto const& coef, std::string_view str) {
			if (coef == 0)
				return;
			was = true;
			if (coef > 0)
				sstream << " + ";
			else
				sstream << " - ";
			if (auto abs = std::abs(coef); abs != 1)
				sstream << abs;
			sstream << str;
		};
		auto const& last = [&](auto const& coef) {
			if (coef == 0)
			{
				if (!was)
					sstream << "0";
				return;
			}
			// TODO handle was again
			if (coef > 0)
				sstream << " + ";
			else
				sstream << " - ";
			sstream << std::abs(coef);
		};
		first(y2, "y^2");
		next(x2, "x^2");
		next(xy, "xy");
		next(x, "x");
		next(y, "y");
		last(this->c);
		sstream << " = z";
		return sstream.str();
	}

	std::tuple<Vector<T>, Vector<T>, Vector<T>> canonicalCoordSys() const
	{
		auto [a, b, c, d, e] = get2d_coefs();
		auto f = this->c;

		auto center = Vector<T>({2 * c * d - b * e, 2 * a * e - d * b});
		center /= (b * b - 4 * a * c);
		f += d * center[0] + e * center[1];
		f += a * square(center[0]) + b * center[0] * center[1] + c * square(center[1]);

		T tan_alpha = 0, tan2_alpha = 0;

		if (b != 0)
		{
			auto [tan1, tan2] = solveSquare(1.0, 2 * (a - c) / b, -1.0);
			tan_alpha = tan1 > 0 ? tan1 : tan2;
			tan2_alpha = square(tan_alpha);
		}
		auto cos2_alpha = 1 / (1 + tan2_alpha), sin2_alpha = 1 - cos2_alpha, sincos_alpha = tan_alpha * cos2_alpha;

		std::tie(a, c) = std::make_pair(
			a * cos2_alpha + c * sin2_alpha + b * sincos_alpha,
			a * sin2_alpha + c * cos2_alpha - b * sincos_alpha
		);
		a /= -f;
		c /= -f;

		auto v1 = Vector<T>{1, tan_alpha};
		v1 /= Len(v1);
		auto v2 = Vector<T>({-v1[1], v1[0]});

		return {center, v1 / sqrt(a), v2 / sqrt(c)};
	}

	template<typename Random>
	static QuadraticFunction2d Rand(Random const& r)
	{
		return QuadraticFunction2d(r(), r(), r(), r(), r(), r());
	}
};

namespace helper
{
	template<typename T, Matrix<T> MatrixImpl>
	MatrixImpl quadraticFunctionMatrixGetter(QuadraticFunction<T, MatrixImpl>);

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

