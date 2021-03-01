#pragma once

#include "./Approximator.hpp"

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
