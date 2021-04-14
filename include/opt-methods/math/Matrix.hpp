#pragma once

#include <cassert>
#include <valarray>
#include <cmath>

#include "./Vector.hpp"

template<typename T, typename S>
concept Matrix = requires(T& t, T const& ct, Vector<S> const& v) {
	{ ct.transpose() } -> std::same_as<T>;
	{ v * ct } -> std::same_as<Vector<S>>;
	{ ct.dims() } -> std::same_as<size_t>;
};

template<typename T>
class DenseMatrix
{
public:
	std::valarray<T> data;
	size_t n;

	DenseMatrix(std::valarray<T> data)
	: data(std::move(data))
	, n((size_t)sqrt(this->data.size()))
	{}

	DenseMatrix(size_t n)
	: data(n * n)
	, n(n)
	{}

	size_t dims() const { return n; }
	std::slice_array<T> row_(size_t i) { return data[std::slice(n * i, n, 1)]; }
	std::slice_array<T> col_(size_t j) { return data[std::slice(j, n, n)]; }
	std::valarray<T> row_(size_t i) const { return data[std::slice(n * i, n, 1)]; }
	std::valarray<T> col_(size_t j) const { return data[std::slice(j, n, n)]; }
	Vector<T> row(size_t i) const { return Vector<T>(row_(i)); }
	Vector<T> col(size_t j) const { return Vector<T>(col_(j)); }
	DenseMatrix<T> transpose() const
	{
		DenseMatrix<T> res(n);
		for (size_t i = 0; i < n; i++)
			res.col_(i) = static_cast<std::valarray<T>>(row_(i));
		return res;
	}
};

template<typename T>
Vector<T> operator*(DenseMatrix<T> const& l, Vector<T> const& r)
{
	assert(l.n == r.size());
	Vector<T> res(l.n);
	for (size_t i = 0; i < l.n; i++)
		res[i] = dot(l.row(i), r);
	return res;
}

