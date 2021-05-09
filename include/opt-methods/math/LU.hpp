#pragma once

#include "./SkylineMatrix.hpp"
#include <numeric>
#include <valarray>

template<typename T>
SkylineMatrix<T>&& SkylineMatrix<T>::LU() &&
{
	int n = Dims();

	for (int i = 0; i < n; i++)
	{
		for (int jO = 0; jO < ia[i + 1] - ia[i]; jO++)
		{
			int j = SkylineStart(i) + jO, nonzeroOff = std::min(jO, ia[j + 1] - ia[j]); // j < i
			int iBegin = ia[i] + jO - nonzeroOff, jBegin = ia[j] + jO - nonzeroOff;

			al[ia[i] + jO] -= std::transform_reduce(al.begin() + iBegin, al.begin() + ia[i] + jO, au.begin() + jBegin, zero);
			au[ia[i] + jO] -= std::transform_reduce(au.begin() + iBegin, au.begin() + ia[i] + jO, al.begin() + jBegin, zero);
			au[ia[i] + jO] /= di[j];
		}
		di[i] -= std::transform_reduce(al.begin() + ia[i], al.begin() + ia[i + 1], au.begin() + ia[i], zero);
	}

	return std::move(*this);
}

template<typename T>
Vector<T> SkylineMatrix<T>::SolveSystem(const Vector<T>& b) &&
{
	std::move(*this).LU();
	assert(Dims() == (int)b.size());

	Vector<T> y;
	y.resize(Dims());

	for (int i = 0; i < Dims(); i++)
		y[i] = (b[i] - std::transform_reduce(al.begin() + ia[i], al.begin() + ia[i + 1], std::begin(y) + SkylineStart(i), zero)) /
		       di[i];

	Vector<T> x;
	x.resize(Dims());
	for (int i = Dims() - 1; i >= 0; i--)
	{
		auto r = Row(i);
		x[i]   = y[i] - std::transform_reduce(r.IteratorAt(i + 1), r.end(), std::begin(x) + (i + 1), zero);
	}

	return x;
}

