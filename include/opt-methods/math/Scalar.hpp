#pragma once

#include <concepts>

#include "./Vector.hpp"

template<typename T>
struct ScalarImpl;

template<typename T>
	requires std::integral<T> || std::floating_point<T>
struct ScalarImpl<T>
{
	using type = T;
};

template<typename T>
struct ScalarImpl<Vector<T>>
{
	using type = T;
};

template<typename T>
using Scalar = typename ScalarImpl<T>::type;

template<typename T, typename Y>
struct ScalarSubstImpl;

template<typename T, typename Y>
	requires std::integral<T> || std::floating_point<T>
struct ScalarSubstImpl<T, Y>
{
	using type = Y;
};

template<typename T, typename Y>
struct ScalarSubstImpl<Vector<T>, Y>
{
	using type = Vector<Y>;
};

template<typename T, typename Y>
using ScalarSubst = typename ScalarSubstImpl<T, Y>::type;

