#pragma once

#include "./SkylineMatrix.hpp"
#include <numeric>
#include <valarray>

template<typename T>
SkylineMatrix<T>&& SkylineMatrix<T>::LU() &&
{
	int n = dims();

	for (int i = 0; i < n; i++)
	{
		for (int jO = 0; jO < ia[i + 1] - ia[i]; jO++)
		{
			int j = skylineStart(i) + jO;  // j < i

			al[ia[i] + jO] -=
			    std::transform_reduce(al.begin() + ia[i], al.begin() + ia[i] + jO, au.begin() + ia[j], zero);
			au[ia[i] + jO] -=
			    std::transform_reduce(au.begin() + ia[i], au.begin() + ia[i] + jO, al.begin() + ia[j], zero);
			au[ia[i] + jO] /= di[j];
		}
		di[i] -= std::transform_reduce(al.begin() + ia[i], al.begin() + ia[i + 1], au.begin() + ia[i], zero);
	}

	return std::move(*this);
}

template<typename T>
Vector<T> SkylineMatrix<T>::solveSystem(const Vector<T>& b) &&
{
	std::move(*this).LU();
	assert(dims() == b.size());

	Vector<T> y;
	y.resize(dims());

	for (int i = 0; i < dims(); i++)
		y[i] = (b[i] - std::transform_reduce(al.begin() + ia[i], al.begin() + ia[i + 1], std::begin(y) + skylineStart(i), zero)) /
		       di[i];

	Vector<T> x;
	x.resize(dims());
	for (int i = dims() - 1; i >= 0; i--)
	{
		auto r = row(i);
		x[i]   = y[i] - std::transform_reduce(r.iteratorAt(i + 1), r.end(), std::begin(x) + (i + 1), zero);
	}

	return x;
}

