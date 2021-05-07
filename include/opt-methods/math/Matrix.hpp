#pragma once

#include <cassert>
#include <valarray>
#include <cmath>
#include <ranges>
#include <algorithm>
#include <numeric>

#include "./Vector.hpp"

template<typename T, typename S>
concept Matrix = requires(T& t, T const& ct, Vector<S> const& v) {
	{ ct.transpose() } -> std::same_as<T>;
	{ ct * v } -> std::same_as<Vector<S>>;
	{ ct.dims() } -> std::same_as<size_t>;
};

template<typename T>
class DenseMatrix
{
private:
	using VectorIterator = decltype(std::begin(std::declval<Vector<T>>()));
public:
	std::valarray<T> data;
	const size_t n;

	DenseMatrix(size_t n, std::valarray<T> data)
	: data(std::move(data))
	, n(n)
	{
		assert(n * n == this->data.size());
	}

	size_t dims() const { return n; }
	std::slice rowSlice(size_t i, size_t j = 0) { return std::slice(n * i + j, n - j, 1); }
	std::slice colSlice(size_t j, size_t i = 0) { return std::slice(n * i + j, n - i, n); }
	std::slice_array<T> row_(size_t i) { return data[std::slice(n * i, n, 1)]; }
	std::slice_array<T> col_(size_t j) { return data[std::slice(j, n, n)]; }
	std::valarray<T> row_(size_t i) const { return data[std::slice(n * i, n, 1)]; }
	std::valarray<T> col_(size_t j) const { return data[std::slice(j, n, n)]; }
	Vector<T> row(size_t i) const { return Vector<T>(row_(i)); }
	Vector<T> col(size_t j) const { return Vector<T>(col_(j)); }
	DenseMatrix transpose() const
	{
		std::valarray<T> res(n * n);
		for (size_t i = 0; i < n; i++)
			res[std::slice(i, n, n)] = static_cast<std::valarray<T>>(row_(i));
		return DenseMatrix(res);
	}

	std::slice_array<T> operator[](size_t i) { return row(i); }
	std::valarray<T> operator[](size_t i) const { return row(i); }

	auto iteratorAt(size_t i, size_t j) const
	{
		return std::begin(data) + (i * n + j);
	}
	auto iteratorAt(size_t i, size_t j)
	{
		return std::begin(data) + (i * n + j);
	}
	T const& at(size_t i, size_t j) const
	{
		return *iteratorAt(i, j);
	}
	T& at(size_t i, size_t j)
	{
		return *iteratorAt(i, j);
	}

	Vector<T> solveSystem(Vector<T> b) &&
	{
		std::vector<int> pi(n + 1);
		std::iota(pi.begin(), pi.end(), 0);

		for (size_t k = 0; k < n - 1; k++)
		{
			auto a_mk = std::max_element(util::PermutedStridedIterator(std::begin(data), n, k, k, n, pi),
			                             util::PermutedStridedIterator(std::begin(data), n, n, k, 0, pi));
			size_t m  = a_mk.i;
			std::swap(pi[k], pi[m]);

			for (size_t i = k + 1; i < n; i++)
			{
				T t = at(pi[i], k) / at(pi[k], k);
				b[pi[i]] -= t * b[pi[k]];

				std::transform(iteratorAt(pi[i], k + 1),
				               iteratorAt(pi[i] + 1, 0),
				               iteratorAt(pi[k], k + 1),
				               iteratorAt(pi[i], k + 1),
				               [&t](T const& lhs, T const& rhs) { return lhs - t * rhs; });
			}
		}
		Vector<T> x(b.size());
		for (size_t k = n; k > 0; k--)
			x[k - 1] = (b[pi[k - 1]] - std::transform_reduce(
			                               iteratorAt(pi[k - 1], k), iteratorAt(pi[k - 1] + 1, 0), std::begin(x) + k, T{})) /
			           at(pi[k - 1], k - 1);
		return x;
	}
};

template<typename T>
Vector<T> operator*(DenseMatrix<T> const& l, Vector<T> const& r)
{
	assert(l.dims() == r.size());
	Vector<T> res(l.n);
	for (size_t i = 0; i < l.n; i++)
		res[i] = dot(l.row(i), r);
	return res;
}

template<typename T>
class DiagonalMatrix
{
public:
	const std::valarray<T> diag;

	DiagonalMatrix(std::valarray<T> diag)
	: diag(std::move(diag))
	{}

	size_t dims() const { return diag.size(); }
	DiagonalMatrix transpose() const { return *this; }
};

template<typename T>
Vector<T> operator*(DiagonalMatrix<T> const& l, Vector<T> const& r)
{
	assert(l.dims() == r.size());
	return l.diag * r;
}
