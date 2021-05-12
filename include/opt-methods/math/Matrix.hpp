#pragma once

#include <cassert>
#include <valarray>
#include <cmath>
#include <ranges>
#include <algorithm>
#include <numeric>
#include <concepts>
#include <functional>

#include "./Vector.hpp"

template<typename T, typename S>
concept Matrix = requires(T& t, T const& ct, Vector<S> const& v) {
	// { ct.Transpose() } -> std::same_as<T>;
	{ ct * v } -> std::same_as<Vector<S>>;
	{ ct.Dims() } -> std::convertible_to<size_t>;
};

template<typename T, typename S>
concept SLESolver = Matrix<T, S> && requires(T&& t, Vector<S> const& b) {
	{ std::move(t).SolveSystem(b) } -> std::same_as<Vector<S>>;
};

namespace util
{
	template<typename T, Matrix<T> M>
	class MatrixGenerator
	{
		template<typename TT, Matrix<TT> MT>
		friend class MatrixGenerator;

	public:
		mutable std::default_random_engine engine{};
	private:
		MatrixGenerator(std::default_random_engine engine)
		: engine(std::move(engine))
		{}

	public:
		MatrixGenerator() = default;

		template<Matrix<T> Mu>
		operator MatrixGenerator<T, Mu>() const
		{
			return MatrixGenerator<T, Mu>(engine);
		}
	};

	template<typename T, Matrix<T> M>
	M DiagonallyDominant(MatrixGenerator<T, M> const& gen, size_t n, T dominance,
		                   std::vector<ptrdiff_t> const& selectedDiagonals,
		                   std::invocable<std::default_random_engine&> auto&& aijDistribution);

	template<typename T, Matrix<T> M>
	M Hilbert(MatrixGenerator<T, M> const& gen, size_t n,
		        std::vector<ptrdiff_t> const& selectedDiagonals);
} // namespace util
