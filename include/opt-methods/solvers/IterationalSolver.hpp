#pragma once

#include "./ErasedApproximator.hpp"

template<typename P, typename V>
class IterationalSolver
{
public:
	using Bounds = RangeBounds<P>;
	using BoundsEval = BoundsWithValues<P, V>;

	ErasedApproximator<P, V> approximator;

	template<typename ... Args>
		requires std::is_constructible_v<ErasedApproximator<P, V>, Args&&...>
	IterationalSolver(Args&& ... args)
	: approximator(std::forward<Args>(args)...)
	{}

	template<Function<P, V> F>
	BoundsEval solveIteration(F& func, std::size_t iterations, Bounds bounds)
	{
		BoundsEval b = {{bounds.l, func(bounds.l)}, {bounds.r, func(bounds.r)}};
		approximator.init(func, b);
		while (iterations-- > 0)
			b = approximator();
		return b;
	}
	template<Function<P, V> F>
	BoundsEval solveDiff(F& func, double diff, Bounds bounds)
	{
		using std::norm; // use ADL
		BoundsEval b = {{bounds.l, func(bounds.l)}, {bounds.r, func(bounds.r)}};
		approximator.init(func, b);
		diff *= diff;
		while (true)
			if (auto d1 = b.r.p - b.l.p; norm(d1) < diff)
				break;
			else
				b = approximator();
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

template<typename P, typename V, size_t N>
class IterationalSolverBuilder;

template<typename P, typename V>
class IterationalSolverBuilder<P, V, 0> {};

template<typename P, typename V, size_t N>
class IterationalSolverBuilder : private IterationalSolverBuilder<P, V, N - 1>
{
private:
	using Chained = IterationalSolverBuilder<P, V, N - 1>;
	IterationalSolver<P, V> approximator;
public:
	template<typename T1, typename ... Tuples>
	IterationalSolverBuilder(T1&& t, Tuples&&... tail)
	: Chained(std::forward<Tuples>(tail)...)
	, approximator(ConstructFromTuple<decltype(approximator)>(std::forward<T1>(t)))
	{
		static_assert(sizeof...(tail) == N - 1);
	}
	template<typename Func, typename ... Args>
	void each(Func& f, Args&& ... a)
		requires std::invocable<Func, IterationalSolver<P, V>&, Args...>
	{
		f(approximator, std::forward<Args>(a)...);
		if constexpr (N > 1)
			Chained::each(f, std::forward<Args>(a)...);
	}
};
