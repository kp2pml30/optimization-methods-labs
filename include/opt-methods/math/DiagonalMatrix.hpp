#pragma once

#include <cassert>
#include <valarray>
#include <cmath>
#include <ranges>
#include <algorithm>
#include <numeric>

#include "./Vector.hpp"

template<typename T>
class DiagonalMatrix
{
public:
	const std::valarray<T> diag;

	DiagonalMatrix(std::valarray<T> diag)
	: diag(std::move(diag))
	{}

	size_t Dims() const { return diag.size(); }
	DiagonalMatrix Transpose() const { return *this; }
};

template<typename T>
Vector<T> operator*(DiagonalMatrix<T> const& l, Vector<T> const& r)
{
	assert(l.Dims() == r.size());
	return l.diag * r;
}
