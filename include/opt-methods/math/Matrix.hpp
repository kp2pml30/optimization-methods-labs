#pragma once

#include <cassert>
#include <valarray>
#include <cmath>

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
public:
	const std::valarray<T> data;
	const size_t n;

	DenseMatrix(std::valarray<T> data)
	: data(std::move(data))
	, n((size_t)sqrt(this->data.size()))
	{}

	size_t dims() const { return n; }
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
