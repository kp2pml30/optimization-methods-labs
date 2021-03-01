#pragma once

#include <type_traits>
#include <concepts>
#include <utility>
#include <complex>
#include <tuple>

template<typename Point, typename Value>
struct PointAndValue
{
	using P = Point;
	using V = Value;

	P p; // point
	V v; // value

	PointAndValue(P point, V value)
	: p(std::move(point))
	, v(std::move(value))
	{}

	PointAndValue() = default;
	PointAndValue(PointAndValue const&) = default;
	PointAndValue(PointAndValue&&) = default;
	PointAndValue& operator=(PointAndValue const&) = default;
	PointAndValue& operator=(PointAndValue&&) = default;
};

template<typename T>
struct IsPointAndValue : std::false_type {};
template<typename F, typename T>
struct IsPointAndValue<PointAndValue<F, T>> : std::true_type {};
template<typename T>
constexpr bool IsPointAndValueT = IsPointAndValue<T>::value;

template<typename Y>
struct RangeBounds
{
	Y l; // left
	Y r; // right

	RangeBounds(Y l, Y r)
	: l(std::move(l))
	, r(std::move(r))
	{}

	RangeBounds() = default;
	RangeBounds(RangeBounds const&) = default;
	RangeBounds(RangeBounds&&) = default;
	RangeBounds& operator=(RangeBounds const&) = default;
	RangeBounds& operator=(RangeBounds&&) = default;
};

template<typename From, typename To>
using BoundsWithValues = RangeBounds<PointAndValue<From, To>>;

template<typename T, typename From, typename To>
concept Function = requires(T& t, From const& f) {
	{ t(f) } -> std::same_as<To>;
};

template<typename P, typename V>
struct DummyFunc
{
	V operator()(P const&) { return std::declval<V>(); }
};

template<typename T, typename P, typename V>
concept Approximator = requires(T& t, DummyFunc<P, V> func, BoundsWithValues<P, V> const& bounds) {
	// Function<P, V> func
	typename T::P;
	typename T::V;
	requires Function<decltype(func), P, V>;
	std::is_same_v<typename T::P, P>;
	std::is_same_v<typename T::V, V>;
	{ t(func, bounds) } -> std::same_as<BoundsWithValues<P, V>>;
};

