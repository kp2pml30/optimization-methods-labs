#pragma once

#include "./Vector.hpp"
#include "./Matrix.hpp"
#include "./SkylineMatrix.hpp"
#include "../util/Util.hpp"

#include <cassert>
#include <valarray>
#include <cmath>
#include <ranges>
#include <algorithm>
#include <numeric>

template<typename T>
class DenseMatrix
{
private:
	using VectorIterator = decltype(std::begin(std::declval<Vector<T>>()));
public:
	std::valarray<T> data;
	size_t n;

	DenseMatrix() = default;
	DenseMatrix(size_t n, std::valarray<T> data)
	: data(std::move(data))
	, n(n)
	{
		assert(n * n == this->data.size());
	}

	static DenseMatrix Identity(size_t n)
	{
		DenseMatrix res(n, std::valarray<T>(0.0, n * n));
		res.data[std::slice(0, n, n + 1)] = 1;
		return res;
	}

	DenseMatrix& operator+=(DenseMatrix const& rhs)
	{
		data += rhs.data;
		return *this;
	}
	DenseMatrix& operator-=(DenseMatrix const& rhs)
	{
		data -= rhs.data;
		return *this;
	}
	DenseMatrix operator+(DenseMatrix const& rhs) const
	{
		return DenseMatrix(n, data + rhs.data);
	}
	DenseMatrix operator-(DenseMatrix const& rhs) const
	{
		return DenseMatrix(n, data - rhs.data);
	}
	DenseMatrix operator-() const
	{
		return DenseMatrix(n, -data);
	}
	DenseMatrix operator/(T rhs) const
	{
		return DenseMatrix(n, data / rhs);
	}
	DenseMatrix& operator/=(T rhs)
	{
		data /= rhs;
		return *this;
	}
	DenseMatrix operator*(T rhs) const
	{
		return DenseMatrix(n, data * rhs);
	}
	DenseMatrix& operator*=(T rhs)
	{
		data *= rhs;
		return *this;
	}

	static DenseMatrix TensorProduct(Vector<T> lhs, Vector<T> rhs)
	{
		assert(lhs.size() == rhs.size());
		size_t n = lhs.size();
		DenseMatrix res(n, std::valarray<T>(0.0, n * n));
		for (int i = 0; i < (int)n; i++)
			for (int j = 0; j < (int)n; j++)
				res.data[i * n + j] = lhs[i] * rhs[j];
		return res;
	}

	size_t Dims() const { return n; }
	std::slice RowSlice(size_t i, size_t j = 0) { return std::slice(n * i + j, n - j, 1); }
	std::slice ColSlice(size_t j, size_t i = 0) { return std::slice(n * i + j, n - i, n); }
	std::slice_array<T> Row_(size_t i) { return data[std::slice(n * i, n, 1)]; }
	std::slice_array<T> Col_(size_t j) { return data[std::slice(j, n, n)]; }
	std::valarray<T> Row_(size_t i) const { return data[std::slice(n * i, n, 1)]; }
	std::valarray<T> Col_(size_t j) const { return data[std::slice(j, n, n)]; }
	Vector<T> Row(size_t i) const { return Vector<T>(Row_(i)); }
	Vector<T> Col(size_t j) const { return Vector<T>(Col_(j)); }
	DenseMatrix Transpose() const
	{
		std::valarray<T> res(n * n);
		for (size_t i = 0; i < n; i++)
			res[std::slice(i, n, n)] = static_cast<std::valarray<T>>(Row_(i));
		return DenseMatrix(res);
	}

	std::slice_array<T> operator[](size_t i) { return Row(i); }
	std::valarray<T> operator[](size_t i) const { return Row(i); }

	auto IteratorAt(size_t i, size_t j) const
	{
		return std::begin(data) + (i * n + j);
	}
	auto IteratorAt(size_t i, size_t j)
	{
		return std::begin(data) + (i * n + j);
	}
	T const& At(size_t i, size_t j) const
	{
		return *IteratorAt(i, j);
	}
	T& At(size_t i, size_t j)
	{
		return *IteratorAt(i, j);
	}

	T Det() &&
	{
		std::vector<int> pi(n + 1);
		std::iota(pi.begin(), pi.end(), 0);

		T res = 1;

		for (size_t k = 0; k < n - 1; k++)
		{
			auto [a_mk1, a_mk2] = std::minmax_element(util::PermutedStridedIterator(std::begin(data), n, k, k, n, pi),
			                                          util::PermutedStridedIterator(std::begin(data), n, n, k, 0, pi));
			using std::abs;
			size_t m  = abs(*a_mk1) > abs(*a_mk2) ? a_mk1.i : a_mk2.i;
			if (k != m)
			{
				std::swap(pi[k], pi[m]);
				res *= -1;
			}

			for (size_t i = k + 1; i < n; i++)
			{
				T t = At(pi[i], k) / At(pi[k], k);

				std::transform(IteratorAt(pi[i], k + 1),
				               IteratorAt(pi[i] + 1, 0),
				               IteratorAt(pi[k], k + 1),
				               IteratorAt(pi[i], k + 1),
				               [&t](T const& lhs, T const& rhs) { return lhs - t * rhs; });
			}
		}
		for (std::size_t i = 0; i < n; i++)
			res *= data[pi[i] * n + i];
		return res;
	}

	T Det() const&
	{
		auto copy = *this;
		return std::move(copy).Det();
	}

	void MinorMatrixTo(DenseMatrix&& res, const std::size_t col, const std::size_t row) const
	{
		assert(res.Dims() == n - 1);
		std::size_t ind = 0;
		for (std::size_t y = 0; y < n; y++)
		{
			if (y == row)
				continue;
			for (std::size_t x = 0; x < n; x++)
				if (x != col)
					res.data[ind++] = data[y * n + x];
		}
	}
	DenseMatrix MinorMatrix(const std::size_t col, const std::size_t row) const
	{
		auto answer = DenseMatrix(n - 1, std::valarray<T>((n - 1) * (n - 1)));
		MinorMatrixTo(std::move(answer), col, row);
		return answer;
	}

	DenseMatrix Inverse() const
	{
		auto res = DenseMatrix(n, std::valarray<T>(n * n));
		auto minor = DenseMatrix(n - 1, std::valarray<T>((n - 1) * (n - 1)));
		for (std::size_t y = 0; y < n; y++)
			for (std::size_t x = 0; x < n; x++)
			{
				MinorMatrixTo(std::move(minor), y, x);
				res.data[y * n + x] = ((int)(y + x) % 2 * -2 + 1) * std::move(minor).Det();
			}
		res.data /= Det();
		return res;
	}

	Vector<T> SolveSystem(Vector<T> b) &&
	{
		std::vector<int> pi(n + 1);
		std::iota(pi.begin(), pi.end(), 0);

		for (size_t k = 0; k < n - 1; k++)
		{
			auto [a_mk1, a_mk2] = std::minmax_element(util::PermutedStridedIterator(std::begin(data), n, k, k, n, pi),
			                                          util::PermutedStridedIterator(std::begin(data), n, n, k, 0, pi));
			using std::abs;
			size_t m  = abs(*a_mk1) > abs(*a_mk2) ? a_mk1.i : a_mk2.i;
			std::swap(pi[k], pi[m]);

			for (size_t i = k + 1; i < n; i++)
			{
				T t = At(pi[i], k) / At(pi[k], k);
				b[pi[i]] -= t * b[pi[k]];

				std::transform(IteratorAt(pi[i], k + 1),
				               IteratorAt(pi[i] + 1, 0),
				               IteratorAt(pi[k], k + 1),
				               IteratorAt(pi[i], k + 1),
				               [&t](T const& lhs, T const& rhs) { return lhs - t * rhs; });
			}
		}
		Vector<T> x(b.size());
		for (size_t k = n; k > 0; k--)
			x[k - 1] = (b[pi[k - 1]] - std::transform_reduce(
			                               IteratorAt(pi[k - 1], k), IteratorAt(pi[k - 1] + 1, 0), std::begin(x) + k, T{})) /
			           At(pi[k - 1], k - 1);
		return x;
	}

	void WriteTo(std::filesystem::path const& p) const
	{
		using namespace util;
		std::ofstream o(p / "data.txt");
		o.precision(15);
		o << n << '\n' << data << '\n';
	}

	static DenseMatrix<T> ReadFrom(std::filesystem::path const& p)
	{
		using namespace util;
		std::ifstream i(p / "data.txt");
		DenseMatrix<T> res;
		i >> res.n >> res.data;
		return res;
	}
};

template<typename T>
std::ostream& PrintDense(std::ostream& o, DenseMatrix<T> const& m)
{
	o << m.Dims() << '\n';
	for (int y = 0; y < (int)m.Dims(); y++)
	{
		auto row = m[y];
		if (m.Dims() > 0)
			o << row[0];
		for (int x = 1; x < (int)m.Dims(); x++)
			o << '\t' << row[x];
		o << '\n';
	}
	return o;
}

template<typename T>
DenseMatrix<T> operator+(DenseMatrix<T> l, DenseMatrix<T> const& r)
{
	assert(l.Dims() == r.Dims());
	l.data += r.data;
	return l;
}

template<typename T>
Vector<T> operator*(DenseMatrix<T> const& l, Vector<T> const& r)
{
	assert(l.Dims() == r.size());
	Vector<T> res(l.n);
	for (size_t i = 0; i < l.n; i++)
		res[i] = Dot(l.Row(i), r);
	return res;
}

namespace util
{
	template<typename T>
	DenseMatrix<T> DiagonallyDominant(MatrixGenerator<T, DenseMatrix<T>> const& gen,
	                                  size_t n,
	                                  T dominance,
	                                  std::vector<ptrdiff_t> const& selectedDiagonals,
	                                  std::invocable<std::default_random_engine&> auto&& aijDistribution)
	{
		return static_cast<DenseMatrix<T>>(DiagonallyDominant(
		    static_cast<MatrixGenerator<T, SkylineMatrix<T>>>(gen), n, dominance, selectedDiagonals, aijDistribution));
	}

	template<typename T>
	DenseMatrix<T> Hilbert(MatrixGenerator<T, DenseMatrix<T>> const& gen,
	                       size_t n,
	                       std::vector<ptrdiff_t> const& selectedDiagonals)
	{
		return static_cast<DenseMatrix<T>>(
		    Hilbert(static_cast<MatrixGenerator<T, SkylineMatrix<T>>>(gen), n, selectedDiagonals));
	}
}
