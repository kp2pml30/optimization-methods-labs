#pragma once

#include <type_traits>
#include <concepts>
#include <utility>
#include <complex>
#include <tuple>
#include <cassert>

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
{};

/** coroutines promise for approximators
 */
template<typename P, typename V>
struct ApproxPromise : Promise<BoundsWithValues<P, V>, ApproxPromise<P, V>>
{
	using Super = Promise<BoundsWithValues<P, V>, ApproxPromise<P, V>>;

	std::unique_ptr<BaseIterationData<P, V>> data{};
	std::function<std::unique_ptr<BaseIterationData<P, V>>(BaseIterationData<P, V> *)> copier;

	std::suspend_never initial_suspend() noexcept { return {}; } // skip first yield resulting in data ptr

	using Super::yield_value;
	/**
	 * data setter
	 */
	template<std::derived_from<BaseIterationData<P, V>> IterationData>
	std::suspend_always yield_value(IterationData* data) requires !std::is_same_v<std::remove_cvref_t<IterationData>, BoundsWithValues<P, V>>
	{
		this->data = std::unique_ptr<IterationData>(data);
		copier     = [](BaseIterationData<P, V>* data) -> std::unique_ptr<BaseIterationData<P, V>> {
			return std::make_unique<IterationData>(static_cast<IterationData &>(*data));
		};
		return {};
	}

	/**
	 * data getter
	 */
	BaseIterationData<P, V> const& getIterationData()
	{
		assert(data != nullptr);
		return *data;
	}

	/**
	 * data copy getter
	 */
	std::unique_ptr<BaseIterationData<P, V>> getIterationDataCopy()
	{
		assert(data != nullptr);
		return copier(data.get());
	}
};

/** coroutines generator for approximators
 */
template<typename P, typename V>
class ApproxGenerator : public Generator<BoundsWithValues<P, V>, ApproxPromise<P, V>>
{
private:
	using Super = Generator<BoundsWithValues<P, V>, ApproxPromise<P, V>>;

public:
	using Super::Generator;

	ApproxGenerator(ApproxGenerator&& s) = default;
	ApproxGenerator(const ApproxGenerator& s) = delete;
	ApproxGenerator& operator=(ApproxGenerator&& s) = default;
	ApproxGenerator& operator=(const ApproxGenerator& s) = delete;

	/**
	 * data getter
	 */
	BaseIterationData<P, V> const& getIterationData() { return this->handle.promise().getIterationData(); }

	/**
	 * data copy getter
	 */
	std::unique_ptr<BaseIterationData<P, V>> getIterationDataCopy() { return this->handle.promise().getIterationDataCopy(); }
};

/**
 * helper concept: has point and value typenames
 */
template<typename T, typename P, typename V>
concept HasPV = requires(T& t) {
	typename T::P;
	typename T::V;
	std::is_same_v<typename T::P, P>;
	std::is_same_v<typename T::V, V>;
};

/**
 * helper concept: hasPV + has IterationalData typename
 */
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


template<typename From, typename To, typename CRTP_Child>
class BaseApproximator; // forward declaration

/**
 * Approximator implementation concept, must inherint BaseApproximator
 */
template<typename T, typename P, typename V>
concept ApproximatorImpl = requires(T& t, DummyFunc<P, V> func,
																		BoundsWithValues<P, V> bounds) {
	requires HasIterationData<T, P, V>;
	requires Function<decltype(func), P, V>;
	requires std::is_base_of_v<BaseApproximator<P, V, T>, T>;
	{ t(func, bounds) } -> std::same_as<ApproxGenerator<P, V>>;
};

/**
 * concept for drawable with QChart objects
 */
template<typename T, typename P, typename V>
concept Drawable = requires(T& t, BoundsWithValues<P, V> bounds, QtCharts::QChart& chart) {
	{ t.draw(bounds, std::declval<BaseIterationData<P, V>>(), chart) };
};

/**
 * most abstract Approximator concept
 */
template<typename T, typename P, typename V>
concept Approximator = requires(T& t, DummyFunc<P, V> func, BoundsWithValues<P, V> bounds) {
	// Function<P, V> func
	requires HasPV<T, P, V>;
	requires Drawable<T, P, V>;
	requires Function<decltype(func), P, V>;
	// approximating function
	{ t(func, bounds) } -> std::same_as<ApproxGenerator<P, V>>;
};

