#pragma once

#include "./Erased.hpp"

/**
 * wrapper class to solve with loops
 */
template<typename P, typename V, Approximator<P, V> Approx>
class IterationalSolver
{
public:
	using Bounds = RangeBounds<P>;
	using BoundsEval = PointRegion<P>;

	using SolveData = std::vector<std::pair<std::unique_ptr<BaseIterationData<P, V>>, PointRegion<P>>>;

	Approx approximator;

	template<typename ... Args>
		requires std::is_constructible_v<Approx, Args&&...>
	IterationalSolver(Args&& ... args)
	: approximator(std::forward<Args>(args)...)
	{}
	IterationalSolver()
		requires std::is_default_constructible_v<Approx>
	= default;

private:
	template<Function<P, V> F, typename Checker>
	BoundsEval solveWhile(F&& func, Bounds bounds, Checker&& checker, SolveData& data) requires
		requires(BoundsEval b, std::size_t iter) {
			{ checker(b, iter) } -> std::convertible_to<bool>;
		}
	{
		BoundsEval b = {bounds.l, bounds.r, bound_tag};
		auto gen = approximator(std::forward<F>(func), b);
		for (std::size_t iterations = 0; gen.next(); iterations++)
		{
			data.push_back({gen.getIterationDataCopy(), b});
			if (!checker(b, iterations)) break;
			b = gen.getValue();
		}
		return b;
	}

public:
	/**
	 * @param iterations -- solve while possible && i=0.. < iterations
	 */
	template<Function<P, V> F>
	BoundsEval solveIteration(F&& func, std::size_t iterations, Bounds bounds, SolveData& data)
	{
		return solveWhile(
				std::forward<F>(func), std::move(bounds), [&](auto&&, std::size_t iter) { return iter < iterations; }, data);
	}
	/**
	 * @param diff -- solve while possible && distance between ends > diff
	 */
	template<Function<P, V> F>
	BoundsEval solveDiff(F&& func, double diff, Bounds bounds, SolveData& data)
	{
		return solveWhile(
				std::forward<F>(func),
				std::move(bounds),
				[&](const BoundsEval& b, [[maybe_unused]] std::size_t iter) {
					return b.r >= diff;
				},
				data);
	}
	/**
	 * @param diff -- solve while possible
	 */
	template<Function<P, V> F>
	BoundsEval solveUntilEnd(F&& func, Bounds bounds, SolveData& data) {
		return solveWhile(
				std::forward<F>(func),
				std::move(bounds), [](...) { return true; },
				data);
	}
};

namespace impl
{
	template<typename T, typename Tuple, size_t ... Is>
	T ConstructFromTuple(Tuple&& tuple, std::index_sequence<Is...>)
	{
		return T{std::get<Is>(std::forward<Tuple>(tuple))...};
	}
	template<typename T, typename Tuple>
	T ConstructFromTuple(Tuple&& tuple)
	{
		return ConstructFromTuple<T>(std::forward<Tuple>(tuple), std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>{});
	}
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
	/**
	 * ctor
	 * @param tuples... -- arguments to constructors
	 */
	template<typename T1, typename ... Tuples>
	IterationalSolverBuilder(T1&& t, Tuples&&... tail)
	: Chained(std::forward<Tuples>(tail)...)
	, approximator(impl::ConstructFromTuple<decltype(approximator)>(std::forward<T1>(t)))
	{
		static_assert(sizeof...(tail) == sizeof...(Tail));
	}

	/**
	 * @param f -- function to call on each approximator
	 * @param a... -- parameters to pass to function after current approximator
	 */
	template<typename Func, typename ... Args>
	void each(Func& f, Args&& ... a)
		requires std::invocable<Func, IterationalSolver<P, V, A>&, Args...>
	{
		f(approximator, std::forward<Args>(a)...);
		if constexpr (sizeof...(Tail) != 0)
			Chained::each(f, std::forward<Args>(a)...);
	}
};
