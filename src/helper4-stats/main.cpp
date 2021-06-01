#include "opt-methods/approximators/all.hpp"
#include "opt-methods/newton/all.hpp"
#include "opt-methods/multidim/all.hpp"

#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/Erased.hpp"

#include "opt-methods/math/BisquareFunction.hpp"

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
	constexpr auto indexHelper(std::index_sequence<N...>)
	{
		return (1 * ... * (I < N ? tuple_size_v<std::tuple_element_t<N, T>> : 1));
	}
	template<std::size_t N, std::size_t I, class T>
	constexpr auto index()
	{
		return N / indexHelper<I, T>(std::make_index_sequence<tuple_size_v<T>>()) %
		       tuple_size_v<std::tuple_element_t<I, T>>;
	}

	template<std::size_t N, class T, std::size_t... I>
	constexpr auto cartesianProduct(T const& t, std::index_sequence<I...>)
	{
		return std::forward_as_tuple(std::get<index<N, I, T>()>(std::get<I>(t))...);
	}

	template<class T, std::size_t... N>
	constexpr auto cartesianProduct(T const& t, std::index_sequence<N...>)
	{
		return std::make_tuple(cartesianProduct<N>(t, std::make_index_sequence<tuple_size_v<T>>())...);
	}
} // namespace impl

template<typename... Tuples>
constexpr auto cartesianProduct(Tuples &&... tuples)
{
	return impl::cartesianProduct(std::make_tuple(std::forward<Tuples>(tuples)...),
	                              std::make_index_sequence<(1 * ... * std::tuple_size_v<Tuples>)>());
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

	auto print_joined = [](std::ostream& o, char delim, auto const& range) -> std::ostream& {
		if (range.size() == 0)
			return o;
		o << *range.begin();
		std::for_each(std::next(range.begin()), range.end(), [&](auto const& i) { o << delim << i; });
		return o;
	};

	constexpr double EPSILON = 1e-4;

	using MApprox = GoldenSectionApproximator<S, V>;

	{
		auto approximators = IterationalSolverBuilder<P,
		                                              V,
		                                              Newton<P, V>,
		                                              NewtonOnedim<P, V, MApprox>,
		                                              NewtonDirection<P, V, MApprox>,
		                                              SteepestDescent<P, V, MApprox>>(
		    std::make_tuple(EPSILON),
		    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))),
		    std::make_tuple(std::make_tuple(EPSILON, MApprox(EPSILON))),
		    std::make_tuple(EPSILON, MApprox(EPSILON)));

		std::set<std::string> names;
		auto testFuncs = [&](auto const& funcsTuple, auto&& test) {
			names.clear();
			auto walker = [&](auto const& approx) {
				std::string name = approx.approximator.name();
				std::replace(name.begin(), name.end(), ' ', '-');
				names.emplace(name);

				auto applier = [&, index = 0](auto const& func) mutable {
					test(index++, name, approx, func);
				};
				std::apply([&](auto&&... x) { (applier(std::forward<decltype(x)>(x)), ...); }, funcsTuple);
			};
			approximators.each(walker);
		};
		
		/*{
			auto localPrefix = prefix / "1.1";
			std::tuple funcs = {
			    QuadraticFunction2d<S>(8, 1, 1, 0, 0, -1),
			    adHocFunction(
			        [](P x) -> V { return std::exp(Len2(x)) + square(x[0]-1)*square(x[1]-1); },
			        [](P x) -> Vector<S> {
				        return {2 * x[0] * std::exp(Len2(x)) + 2 * (x[0]-1)*square(x[1]-1),
				                2 * x[1] * std::exp(Len2(x)) + 2 * square(x[0]-1)*(x[1]-1)};
			        },
			        [](P x) {
				        return DenseMatrix<S>(
				            2,
				            {4 * square(x[0]) * std::exp(Len2(x)) + 2 * std::exp(Len2(x)) + 2 * square(x[1] - 1),
				              4 * x[0] * x[1] * std::exp(Len2(x)) + 4 * (x[0] - 1) * (x[1] - 1),
				              4 * x[0] * x[1] * std::exp(Len2(x)) + 4 * (x[0] - 1) * (x[1] - 1),
				              2 * std::exp(Len2(x)) + 4 * square(x[1]) * std::exp(Len2(x)) + 2 * square(x[0] - 1)});
			        })
			};

			auto iterations = std::vector<std::map<std::string, int>>(std::tuple_size<decltype(funcs)>::value);

			testFuncs(cartesianProduct(funcs, std::make_tuple(P{1., 1.}, P.{3., 3.}, P.{5., 0.})),
			          [&](int index, std::string const& name, auto const& approx, auto const& funcPt) mutable {
				          auto&& [func, pt] = funcPt;

				          auto dir = localPrefix / std::to_string(index);
				          fs::create_directories(dir);
				          auto cout = std::ofstream(dir / (name + "Traj.tsv"));

				          typename std::decay_t<decltype(approx)>::SolveData info;
				          approx.solveUntilEnd(func, PointRegion<P>{pt, 10}, info);
				          for (auto const& i : info)
					          cout << i.second.p[0] << '\t' << i.second.p[1] << '\t' << func(i.second.p) << '\n';

				          iterations[index][name] = (int)info.size();
				          index++;
			          });

			auto cout = std::ofstream(localPrefix / "iters.tsv");

			print_joined(cout, '\t', names) << '\n';
			for (auto const& i : iterations)
				print_joined(cout, '\t', std::ranges::views::transform(i, [](auto& p) { return p.second; })) << '\n';
		}*/

		{
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

			testFuncs(funcs, [&](int index, std::string const& name, auto const& approx, auto const& funcPt) mutable {
				auto&& [func, pt] = funcPt;

				auto dir = localPrefix / std::to_string(index);
				fs::create_directories(dir);
				auto cout = std::ofstream(dir / (name + "Traj.tsv"));

				typename std::decay_t<decltype(approx)>::SolveData info;
				approx.solveUntilEnd(func, PointRegion<P>{pt, 10}, info);
				const auto nth = std::max((std::size_t)1, info.size() / 1000);
				for (std::size_t ii = 0; ii < info.size(); ii += nth)
				{
					auto const& i = info[ii];
					cout << i.second.p[0] << '\t' << i.second.p[1] << '\t' << func(i.second.p) << '\n';
				}

				iterations[index][name] = (int)info.size();
				index++;
			});

			int index = 0;
			for (auto const& i : iterations)
			{
				auto cout = std::ofstream(localPrefix / std::to_string(index) / "iters.tsv");
				print_joined(cout, '\t', std::ranges::views::transform(i, [](auto& p) { return p.first; })) << '\n';
				print_joined(cout, '\t', std::ranges::views::transform(i, [](auto& p) { return p.second; })) << '\n';
				index++;
			}
		}
	}
	return 0;
}
