#pragma once

#include <concepts>
#include <valarray>

template<typename T>
struct ScalarImpl;

template<typename T>
	requires std::integral<T> || std::floating_point<T>
struct ScalarImpl<T>
{
	using type = T;
};

template<typename T>
struct ScalarImpl<std::valarray<T>>
{
	using type = T;
};

template<typename T>
using Scalar = ScalarImpl<T>::type;
