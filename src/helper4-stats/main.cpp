#include "opt-methods/approximators/all.hpp"
#include "opt-methods/newton/all.hpp"
#include "opt-methods/multidim/all.hpp"
#include "opt-methods/quasi-newton/all.hpp"

#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/Erased.hpp"

#include "opt-methods/math/BisquareFunction.hpp"

#include <array>
#include <tuple>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <ranges>

template<typename From, typename To,
         Function<From, To> Func,
         Function<From, Vector<Scalar<From>>> Grad,
         Function<From, DenseMatrix<Scalar<From>>> Hessian>
struct AdHocFunction
{
	using S = Scalar<From>;

	Func func_;
	Grad grad_;
	Hessian hessian_;

	AdHocFunction(TypeTag<From>, TypeTag<To>, Func&& func, Grad&& grad, Hessian&& hessian)
	: func_(std::move(func))
	, grad_(std::move(grad))
	, hessian_(std::move(hessian))
	{}

	To operator()(From pt) const
	{
		return func_(pt);
	}

	auto const& grad() const
	{
		return grad_;
	}

	auto const& hessian() const
	{
		return hessian_;
	}
};

namespace impl
{
	template<class T>
	constexpr std::size_t tuple_size_v = std::tuple_size_v<std::decay_t<T>>;

	template<std::size_t I, class T, std::size_t... N>
	consteval auto IndexHelper(std::index_sequence<N...>)
	{
		return (1 * ... * (I < N ? tuple_size_v<std::tuple_element_t<N, T>> : 1));
	}

	template<std::size_t N, std::size_t I, class T>
	consteval auto Index()
	{
		return N / IndexHelper<I, T>(std::make_index_sequence<tuple_size_v<T>>()) %
		       tuple_size_v<std::tuple_element_t<I, T>>;
	}

	template<std::size_t N, class T, std::size_t... I>
	constexpr auto ExtractRepresentatives(std::index_sequence<I...>, T&& t)
	{
		return std::forward_as_tuple(std::get<Index<N, I, T>()>(std::get<I>(std::forward<T>(t)))...);
	}

	template<std::size_t N, class T, std::size_t... I>
	consteval auto ExtractIndices(std::index_sequence<I...>)
	{
		return std::make_tuple(Index<N, I, T>()...);
	}

	template<class T, std::size_t... N>
	constexpr void ApplyCartesianProduct(std::index_sequence<N...>, auto&& func, T&& t)
	{
		using Seq = std::make_index_sequence<tuple_size_v<T>>;
		(std::apply(func, std::tuple_cat(std::make_tuple(ExtractIndices<N, T>(Seq())), ExtractRepresentatives<N>(Seq(), std::forward<T>(t)))), ...);
	}
} // namespace impl

template<typename... Tuples>
constexpr auto ApplyCartesianProduct(auto&& func, Tuples &&... tuples)
{
	return impl::ApplyCartesianProduct(std::make_index_sequence<(1 * ... * impl::tuple_size_v<Tuples>)>(),
	                                   std::forward<decltype(func)>(func),
	                                   std::forward_as_tuple(std::forward<Tuples>(tuples)...));
}

template<typename... Tuples>
void TestSolvers(auto&& solvers, auto&& test, Tuples &&... argTuples) {
	auto walker = [&](auto const& approx) {
		std::string name = approx.approximator.name();
		std::replace(name.begin(), name.end(), ' ', '-');

		ApplyCartesianProduct(
		    [&](auto&& indexTuple, auto&&... args) mutable {
			    test(std::forward<decltype(indexTuple)>(indexTuple), name, approx, std::forward<decltype(args)>(args)...);
		    },
		    argTuples...);
	};
	solvers.each(walker);
}

template<size_t N = 0, class Tuple>
void TupleRuntimeVisit(auto&& func, Tuple&& tuple, size_t idx)
{
	if (N == idx)
	{
		std::invoke(func, std::get<N>(std::forward<Tuple>(tuple)));
		return;
	}

	if constexpr (N + 1 < std::tuple_size_v<std::remove_cvref_t<Tuple>>)
	{
		return TupleRuntimeVisit<N + 1>(func, std::forward<Tuple>(tuple), idx);
	}
}

std::ostream& PrintJoined(std::ostream& o, char delim, auto const& range) {
	if (range.size() == 0)
		return o;
	o << *std::begin(range);
	std::for_each(std::next(std::begin(range)), std::end(range), [&](auto const& i) { o << delim << i; });
	return o;
}

struct Nop
{
	void operator()(...) const {}
};

inline constexpr Nop nop;

void SolveAndPrintTraj(std::filesystem::path const& dir, std::string const& name, auto const& approx, auto const& func, auto const& pt, auto& info)
{
	namespace fs = std::filesystem;
	fs::create_directories(dir);
	auto cout = std::ofstream(dir / (name + "Traj.tsv"));

	approx.solveIteration(func, 10'000, PointRegion{pt, 10}, info);
	const auto nth = std::max((std::size_t)1, info.size() / 1000);
	for (std::size_t ii = 0; ii < info.size(); ii += nth)
	{
		auto const& i = info[ii];
		PrintJoined(cout, '\t', i.second.p) << '\t' << func(i.second.p) << '\n';
	}
}

template<typename Action = Nop const&>
auto TestSolversFuncsPts(
    std::filesystem::path const& localPrefix, auto&& solvers, auto&& funcs, auto&& pts, Action&& actions = nop, size_t fOff = 0, size_t pOff = 0)
{
	namespace fs = std::filesystem;

	auto iterations = std::vector<std::vector<std::map<std::string, int>>>(
	    impl::tuple_size_v<decltype(funcs)>, std::vector<std::map<std::string, int>>(impl::tuple_size_v<decltype(pts)>));
	TestSolvers(
	    solvers,
	    [&]<class Approx, class Func, class Pt>(std::tuple<size_t, size_t> index,
	        std::string const& name,
	        Approx const& approx,
	        Func const& func,
	        Pt const& pt) {
		    auto [fi, pti] = index;

				auto dir = localPrefix / std::to_string(fi + fOff) / std::to_string(pti + pOff);

		    typename std::decay_t<decltype(approx)>::SolveData info;
		    SolveAndPrintTraj(dir, name, approx, func, pt, info);

		    actions(fi, pti, name, dir, approx, func, pt, info);

		    iterations[fi][pti][name] = (int)info.size();
	    },
	    funcs,
	    pts);
	int index = 0;
	for (auto const& i : iterations)
	{
		auto cout = std::ofstream(localPrefix / std::to_string(index + fOff) / "iters.tsv");
		cout << "start\t";
		PrintJoined(cout, '\t', std::ranges::views::transform(i[0], [](auto& p) { return p.first; })) << '\n';
		{
			size_t index = 0;
			for (auto const& ni : i)
			{
				TupleRuntimeVisit([&](auto const& pt) { PrintJoined(cout, ',', pt) << '\t'; }, pts, index);
				PrintJoined(cout, '\t', std::ranges::views::transform(ni, [](auto& p) { return p.second; })) << '\n';
				index++;
			}
		}
		index++;
	}
}


template<typename IterationData>
concept ContainsAlpha = requires(IterationData d) {
	d.alpha;
};

template<typename F, typename R, typename... Args>
concept ReturnsSameAs = requires(F f, Args&&... args) {
	{ f(std::forward<Args>(args)...) } -> std::same_as<R>;
};

template<typename F, typename R, typename S, typename Is>
struct IsNInvocableHelper;

template<typename F, typename R, typename S, size_t... I>
struct IsNInvocableHelper<F, R, S, std::index_sequence<I...>> :
	std::bool_constant<
	  std::is_invocable_v<F, std::conditional_t<true, S, std::array<int, I>>...> &&
	  ReturnsSameAs<F, R, std::conditional_t<true, S, std::array<int, I>>...>
	>
{
};

template<typename F, typename R, size_t N, typename S>
struct IsNInvocable : IsNInvocableHelper<F, R, S, std::make_index_sequence<N>>
{
};

template<typename F, typename R, size_t N, typename S>
concept NInvocable = IsNInvocable<F, R, N, S>::value;

template<typename S, size_t N>
auto flatAdHocFunction(NInvocable<S, N, S> auto&& func, NInvocable<Vector<S>, N, S> auto&& grad,
	                     NInvocable<DenseMatrix<S>, N, S> auto&& hessian)
{
	auto vecIndApply = [&]<size_t... I>(std::index_sequence<I...>, auto &&f, Vector<S> const& x) {
		assert(x.size() == N);
		return f(x[I]...);
	};

	auto vecApply = [&](auto &&f) {
		return [&](Vector<S> const& x) { return vecIndApply(std::make_index_sequence<N>(), f, x); };
	};

	return AdHocFunction(typeTag<Vector<S>>, typeTag<S>, vecApply(func), vecApply(grad), vecApply(hessian));
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr << "expected output path prefix" << std::endl;
		return 1;
	}

	namespace fs = std::filesystem;
	fs::path prefix = argv[1];

	std::cout << std::setprecision(std::numeric_limits<double>::digits10 + 1);

	using V = double;
	using S = double;
	using P = Vector<S>;

	auto adHocFunction = [](Function<P, V> auto&& f, Function<P, Vector<S>> auto&& g, Function<P, DenseMatrix<S>> auto&& h) {
		return AdHocFunction(typeTag<P>,
		                     typeTag<V>,
		                     std::forward<decltype(f)>(f),
		                     std::forward<decltype(g)>(g),
		                     std::forward<decltype(h)>(h));
	};

	constexpr double EPSILON = 1e-5;

	using MApprox = GoldenSectionApproximator<S, V>;

	{
		{
			/* 1.1 */
			auto solvers = IterationalSolverBuilder<P,
			                                        V,
			                                        Newton<P, V>,
			                                        NewtonOnedim<P, V, MApprox>,
			                                        NewtonDirection<P, V, MApprox>>(
			    std::make_tuple(EPSILON),
			    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))),
			    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))));

			auto localPrefix = prefix / "1.1";
			std::tuple funcs = {
			    QuadraticFunction2d<S>(8, 1, 1, 0, 0, -1),
			    adHocFunction(
			        [](P x) -> V { return -std::exp(-Len2(x)) + square(x[0]) + 2 * square(x[1]); },
			        [](P x) -> Vector<S> {
				        return {2 * x[0] * std::exp(-Len2(x)) + 2 * x[0],
				                2 * x[1] * std::exp(-Len2(x)) + 4 * x[1]};
			        },
			        [](P x) {
				        return DenseMatrix<S>(
				            2,
				            {-4 * square(x[0]) * std::exp(-Len2(x)) + 2 * std::exp(-Len2(x)) + 2,
				             -4 * x[0] * x[1] * std::exp(-Len2(x)),
				             -4 * x[0] * x[1] * std::exp(-Len2(x)),
				             -4 * square(x[1]) * std::exp(-Len2(x)) + 2 * std::exp(-Len2(x)) + 4});
			        })
			};

			std::tuple pts = {P{0.5, 0.5}, P{1., 1.}, P{3., 3.}};

			TestSolversFuncsPts(localPrefix,
			                    solvers,
			                    funcs,
			                    pts,
			                    [&](size_t, size_t, std::string const& name, auto const& dir, auto const& approx,
			                        auto const&, auto const&, auto const& info) {
				                    using MIterationData = std::remove_cvref_t<decltype(approx)>::ApproxT::IterationData;
				                    if constexpr (ContainsAlpha<MIterationData>)
				                    {
					                    auto cout = std::ofstream(dir / (name + "Alpha.tsv"));

					                    for (auto const& i : info)
						                    cout << static_cast<MIterationData&>(*i.first).alpha << '\n';
				                    }
			                    });
		}

		{
			/* 1.2 */
			auto solvers = IterationalSolverBuilder<P,
			                                        V,
			                                        Newton<P, V>,
			                                        NewtonOnedim<P, V, MApprox>,
			                                        NewtonDirection<P, V, MApprox>,
			                                        SteepestDescent<P, V, MApprox>>(
			    std::make_tuple(EPSILON),
			    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))),
			    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))),
			    std::make_tuple(EPSILON, MApprox(EPSILON)));

			auto localPrefix = prefix / "1.2";
			std::tuple funcs = {
			    std::make_tuple(QuadraticFunction2d<S>(1, -1.2, 1, 0, 0, 0), P{4., 1.}),
			    std::make_tuple(
			        adHocFunction(
			            [](P x) -> V { return 100 * square(x[1] - square(x[0])) + square(1 - x[0]); },
			            [](P x) -> Vector<S> {
				            return {2 * (200 * cube(x[0]) - 200 * x[0] * x[1] + x[0] - 1), 200 * (x[1] - square(x[0]))};
			            },
			            [](P x) {
				            return DenseMatrix<S>(
				                2, {-400 * (x[1] - square(x[0])) + 800 * square(x[0]) + 2, -400 * x[0], -400 * x[0], 200});
			            }),
			        P{-1.2, 1})
			};

			auto iterations = std::vector<std::map<std::string, int>>(std::tuple_size<decltype(funcs)>::value);
			std::set<std::string> names;

			TestSolvers(
			    solvers,
			    [&](std::tuple<size_t> index, std::string const& name, auto const& approx, auto const& funcPt) mutable {
				    auto&& [func, pt] = funcPt;
				    auto [fi] = index;

				    auto dir = localPrefix / std::to_string(fi);
				    fs::create_directories(dir);
				    auto cout = std::ofstream(dir / (name + "Traj.tsv"));

				    typename std::decay_t<decltype(approx)>::SolveData info;
				    SolveAndPrintTraj(dir, name, approx, func, pt, info);

				    iterations[fi][name] = (int)info.size();
			    },
			    funcs);

			int index = 0;
			for (auto const& i : iterations)
			{
				auto cout = std::ofstream(localPrefix / std::to_string(index) / "iters.tsv");
				PrintJoined(cout, '\t', std::ranges::views::transform(i, [](auto& p) { return p.first; })) << '\n';
				PrintJoined(cout, '\t', std::ranges::views::transform(i, [](auto& p) { return p.second; })) << '\n';
				index++;
			}
		}

		{
			constexpr double EPSILON = 1e-6;

			/* 2 */
			auto solvers = IterationalSolverBuilder<P,
			                                        V,
			                                        QuasiNewtonBFS<P, V, MApprox>,
			                                        QuasiNewtonPowell<P, V, MApprox>,
			                                        NewtonDirection<P, V, MApprox>>( /// FAILS
			    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))),
			    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))),
			    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))));

			auto localPrefix = prefix / "2";
			std::tuple funcs2 = {
			    adHocFunction([](P x) -> V { return 100 * square(x[1] - square(x[0])) + square(1 - x[0]); },
			                  [](P x) -> Vector<S> {
				                  return {2 * (200 * cube(x[0]) - 200 * x[0] * x[1] + x[0] - 1), 200 * (x[1] - square(x[0]))};
			                  },
			                  [](P x) {
				                  return DenseMatrix<S>(
				                      2,
				                      {-400 * (x[1] - square(x[0])) + 800 * square(x[0]) + 2, -400 * x[0], -400 * x[0], 200});
			                  }),
			    adHocFunction([](P x) -> V { return square(square(x[0]) + x[1] - 11) + square(x[0] + square(x[1]) - 7); },
			                  [](P x) -> Vector<S> {
				                  return {2 * (2 * x[0] * (square(x[0]) + x[1] - 11) + x[0] + square(x[1]) - 7),
				                          2 * (square(x[0]) + 2 * x[1] * (x[0] + square(x[1]) - 7) + x[1] - 11)};
			                  },
			                  [](P x) {
				                  return DenseMatrix<S>(2,
				                                        {4 * (square(x[0]) + x[1] - 11) + 8 * square(x[0]) + 2,
				                                         4 * (x[0] + x[1]),
				                                         4 * (x[0] + x[1]),
				                                         4 * (x[0] + square(x[1]) - 7) + 8 * square(x[1]) + 2});
			                  }),
			    flatAdHocFunction<S, 2>(
			        [](S x, S y) -> V {
				        return 100 - 2 / (1 + square((x - 1) / 2) + square((y - 1) / 3)) -
				               1 / (1 + square((x - 2) / 2) + square((y - 1) / 3));
			        },
			        [](S x, S y) -> Vector<S> {
				        return {(-2 + x) / (2 * square(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) +
				                    (-1 + x) / square(1 + square(-1 + x) / 4 + square(-1 + y) / 9),
				                (2 * (-1 + y)) / (9 * square(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) +
				                    (4 * (-1 + y)) / (9 * square(1 + square(-1 + x) / 4 + square(-1 + y) / 9))};
			        },
			        [](S x, S y) -> DenseMatrix<S> {
				        return {2,
				                {-square(-2 + x) / (2 * cube(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) +
				                     1 / (2 * square(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) -
				                     square(-1 + x) / cube(1 + square(-1 + x) / 4 + square(-1 + y) / 9) +
				                     1 / square(1 + square(-1 + x) / 4 + square(-1 + y) / 9),
				                 (-2 * (-2 + x) * (-1 + y)) / (9 * cube(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) -
				                     (4 * (-1 + x) * (-1 + y)) / (9 * cube(1 + square(-1 + x) / 4 + square(-1 + y) / 9)),
				                 (-2 * (-2 + x) * (-1 + y)) / (9 * cube(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) -
				                     (4 * (-1 + x) * (-1 + y)) / (9 * cube(1 + square(-1 + x) / 4 + square(-1 + y) / 9)),
				                 2 / (9 * square(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) +
				                     4 / (9 * square(1 + square(-1 + x) / 4 + square(-1 + y) / 9)) -
				                     (8 * square(-1 + y)) / (81 * cube(1 + square(-2 + x) / 4 + square(-1 + y) / 9)) -
				                     (16 * square(-1 + y)) / (81 * cube(1 + square(-1 + x) / 4 + square(-1 + y) / 9))}};
			        })
			};
			std::tuple funcs4 = {
			    flatAdHocFunction<S, 4>(
			        [](S x1, S x2, S x3, S x4) -> V {
				        return square(x1 + 10 * x2) + 5 * square(x3 - x4) + quad(x2 - 2 * x3) + 10 * quad(x1 - x4);
			        },
			        [](S x, S y, S z, S t) -> Vector<S> {
				        return {2 * (20 * cube(x - t) + x + 10 * y),
				                4 * (5 * (x + 10 * y) + cube(y - 2 * z)),
				                10 * (z - t) - 8 * cube(y - 2 * z),
				                10 * (-4 * cube(x - t) + t - z)};
			        },
			        [](S x, S y, S z, S t) -> DenseMatrix<S> {
				        return {3,
				                {2 + 120 * square(-t + x),
				                 20,
				                 0,
				                 -120 * square(-t + x),
				                 20,
				                 200 + 12 * square(y - 2 * z),
				                 -24 * square(y - 2 * z),
				                 0,
				                 0,
				                 -24 * square(y - 2 * z),
				                 10 + 48 * square(y - 2 * z),
				                 -10,
				                 -120 * square(-t + x),
				                 0,
				                 -10,
				                 10 + 120 * square(-t + x)}};
			        })
			};

			std::tuple pts2  = {P{0.5, 0.5}, P{1.5, 1.5}, P{3., 3.}};
			std::tuple pts4  = {P{0.5, 0.5, 0.5, 0.5}, P{1.5, 1.5, 1.5, 1.5}, P{3., 3., 3., 3.}};

			TestSolversFuncsPts(localPrefix,
			                    solvers,
			                    funcs2,
			                    pts2);
			TestSolversFuncsPts(localPrefix,
			                    solvers,
			                    funcs4,
			                    pts4,
			                    [](...) {}, impl::tuple_size_v<decltype(funcs2)>);
		}
	}
	return 0;
}
