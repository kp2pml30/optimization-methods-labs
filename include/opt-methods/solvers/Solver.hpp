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

template<typename P, typename V, Approximator<P, V> Approx>
class IterationalSolver
{
public:
	using Bounds = RangeBounds<P>;
	using BoundsEval = BoundsWithValues<P, V>;

	Approx approximator;

	template<typename ... Args>
		requires std::is_constructible_v<Approx, Args&&...>
	IterationalSolver(Args&& ... args)
	: approximator(std::forward<Args>(args)...)
	{}
	IterationalSolver()
		requires std::is_default_constructible_v<Approx>
	= default;

	template<Function<P, V> F>
	BoundsEval solveIteration(F& func, std::size_t iterations, Bounds bounds)
	{
		BoundsEval b = {{bounds.l, func(bounds.l)}, {bounds.r, func(bounds.r)}};
		while (iterations-- > 0)
			b = approximator(func, b);
		return b;
	}
	template<Function<P, V> F>
	BoundsEval solveDiff(F& func, double diff, Bounds bounds)
	{
		using std::norm; // use ADL
		BoundsEval b = {{bounds.l, func(bounds.l)}, {bounds.r, func(bounds.r)}};
		diff *= diff;
		while (true)
			if (auto d1 = b.r.p - b.l.p; norm(d1) < diff)
				break;
			else
				b = approximator(func, b);
		return b;
	}
};

template<typename T, typename Tuple, size_t... Is>
T ConstructFromTuple(Tuple&& tuple, std::index_sequence<Is...> )
{
	return T{std::get<Is>(std::forward<Tuple>(tuple))...};
}

template<typename T, typename Tuple>
T ConstructFromTuple(Tuple&& tuple)
{
	return ConstructFromTuple<T>(std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
}

template<typename P, typename V, Approximator<P, V> ... A>
class IterationalSolverBuilder {};

template<typename P, typename V, Approximator<P, V> A, Approximator<P, V> ... Tail>
class IterationalSolverBuilder<P, V, A, Tail...> : private IterationalSolverBuilder<P, V, Tail...>
{
private:
	using Chained = IterationalSolverBuilder<P, V, Tail...>;
	IterationalSolver<P, V, A> approximator;
public:
	template<typename T1, typename ... Tuples>
	IterationalSolverBuilder(T1&& t, Tuples&&... tail)
	: Chained(std::forward<Tuples>(tail)...)
	, approximator(ConstructFromTuple<decltype(approximator)>(std::forward<T1>(t)))
	{
		static_assert(sizeof...(tail) == sizeof...(Tail));
	}
	template<typename Func, typename ... Args>
	void each(Func& f, Args&& ... a)
		requires std::invocable<Func, IterationalSolver<P, V, A>&, Args...>
	{
		f(approximator, std::forward<Args>(a)...);
		if constexpr (sizeof...(Tail) != 0)
			Chained::each(f, std::forward<Args>(a)...);
	}
};
