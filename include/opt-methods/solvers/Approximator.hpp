#pragma once

#include <type_traits>
#include <concepts>
#include <utility>
#include <complex>
#include <tuple>

#include <QtCharts/QChart>
#include <QtCharts/QScatterSeries>

#include "opt-methods/coroutines/Generator.hpp"

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

template<typename P, typename V>
struct BaseIterationData
{
};

template<typename P, typename V>
class ApproxGenerator : public Generator<BoundsWithValues<P, V>>
{
public:
	using promise_type = typename Generator<BoundsWithValues<P, V>>::promise_type;

private:
	using base_t = Generator<BoundsWithValues<P, V>>;
	std::unique_ptr<BaseIterationData<P, V>> data;
	std::function<std::unique_ptr<BaseIterationData<P, V>>(BaseIterationData<P, V> *)> copier;

public:
	ApproxGenerator(base_t&& gen)
	: base_t(std::move(gen))
	{}

	template<typename IterationData>
	void setData(std::unique_ptr<IterationData> data) requires std::is_base_of_v<BaseIterationData<P, V>, IterationData>
	{
		this->data = std::move(data);
		copier     = [](BaseIterationData<P, V>* data) -> std::unique_ptr<BaseIterationData<P, V>> {
			return std::make_unique<IterationData>(static_cast<IterationData &>(*data));
		};
	}

	BaseIterationData<P, V> const& getIterationData() { return *data; }

	std::unique_ptr<BaseIterationData<P, V>> getIterationDataCopy() { return copier(data.get()); }
};

template<typename T, typename P, typename V>
concept HasPV = requires(T& t) {
	typename T::P;
	typename T::V;
	std::is_same_v<typename T::P, P>;
	std::is_same_v<typename T::V, V>;
};

template<typename T, typename P, typename V>
concept HasIterationData = requires(T& t) {
	requires HasPV<T, P, V>;
	typename T::IterationData;
	std::is_base_of_v<BaseIterationData<P, V>, typename T::IterationData>;
};

template<typename T, typename P, typename V>
concept HasDrawImpl = requires(T& t, BoundsWithValues<P, V> bounds, QtCharts::QChart& chart) {
	requires HasIterationData<T, P, V>;
	{ T::draw_impl(bounds, std::declval<typename T::IterationData>(), chart) };
};

template<typename T, typename P, typename V>
concept ApproximatorImpl = requires(T& t, DummyFunc<P, V> func,
																		BoundsWithValues<P, V> bounds) {
	requires HasIterationData<T, P, V>;
	requires Function<decltype(func), P, V>;
	{ t.begin_impl(func, bounds, std::declval<typename T::IterationData&>()) } -> std::same_as<ApproxGenerator<P, V>>;
};

template<typename T, typename P, typename V>
concept Drawable = requires(T& t, BoundsWithValues<P, V> bounds, QtCharts::QChart& chart) {
	{ t.draw(bounds, std::declval<BaseIterationData<P, V>>(), chart) };
};

template<typename T, typename P, typename V>
concept Approximator = requires(T& t, DummyFunc<P, V> func, BoundsWithValues<P, V> bounds) {
	// Function<P, V> func
	requires HasPV<T, P, V>;
	requires Drawable<T, P, V>;
	requires Function<decltype(func), P, V>;
	{ t(func, bounds) } -> std::same_as<ApproxGenerator<P, V>>;
};

