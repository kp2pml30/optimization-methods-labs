#include <type_traits>
#include <concepts>
#include <array>
#include <string>
#include <vector>
#include <tuple>
#include <functional>
#include <cmath>
#include <ostream>

namespace
{
	template<typename Fn, typename Ret, typename... Args>
	concept RetInvocable = requires(Fn f, Args&&... args)
	{
		{ f(std::forward<Args>(args)...) } -> std::convertible_to<Ret>;
	};

	template<std::size_t e, std::size_t b = 0, std::size_t s = 1>
	struct CompiletimeLoop
	{
	public:
		template<typename T>
		static void Go(T const& func)
		{
			if constexpr (b < e)
			{
				func(std::integral_constant<std::size_t, b>{});
				CompiletimeLoop<e, b + s, s>::Go(func);
			}
		}
	};

	template<char delim = '\t', typename TupleLike>
	void PrintTupleLike(std::ostream& o, TupleLike const& tuple)
	{
		CompiletimeLoop<std::tuple_size<TupleLike>::value>::Go([&]<std::size_t i>(std::integral_constant<std::size_t, i>) {
					if constexpr (i != 0)
						o << delim;
					o << std::get<i>(tuple);
				});
	}

	template<typename ...Args>
	struct Table
	{
		static constexpr std::size_t nCols = sizeof...(Args);
		std::array<std::string, nCols> names;
		std::vector<std::tuple<Args...>> rows;


		template<typename ...A>
		void Add(A&&... a)
		{
			rows.emplace_back(std::forward<A>(a)...);
		}
	
		friend std::ostream& operator<<(std::ostream& o, Table const& table)
		{
			PrintTupleLike(o, table.names);
			o << '\n';
			for (auto const& r : table.rows)
			{
				PrintTupleLike(o, r);
				o << '\n';
			}
			return o;
		}
	};

	template<typename T, typename Arg>
	concept Modifier = RetInvocable<T, Arg, Arg const&>;

	template<typename T, typename... Args>
	struct IsTupleModifiers : std::bool_constant<false>
	{
	};

	template<typename... TArgs, typename... Args> requires (sizeof...(TArgs) == sizeof...(Args))
	struct IsTupleModifiers<std::tuple<TArgs...>, Args...> : std::bool_constant<(Modifier<TArgs, Args> && ...)>
	{
	};

	template<typename T, typename... Args>
	concept Modifiers = IsTupleModifiers<T, Args...>::value;

	template<std::size_t i, typename ...Args>
	bool Advance(std::tuple<Args&...> cur,
	             std::tuple<Args...> const& begins,
	             std::tuple<Args...> const& ends,
	             Modifiers<Args...> auto const& modifiers)
	{
		if constexpr (i == sizeof...(Args))
			return false;
		else
		{
			if (Advance<i + 1>(cur, begins, ends, modifiers))
				return true;
			auto& c = std::get<i>(cur);
			c = std::get<i>(modifiers)(c);
			if (c >= std::get<i>(ends))
			{
				c = std::get<i>(begins);
				return false;
			}
			else
			{
				return true;
			}
		}
	}

	template<typename ...Args>
	void GetStatsVectorizedAdvance(RetInvocable<void, Args const&...> auto const& test,
	                               std::tuple<Args...> begins,
	                               RetInvocable<bool, Args&...> auto const& modifiers)
	{
		do
			std::apply(test, begins);
		while (std::apply(modifiers, begins));
	}

	template<typename ...Args>
	void GetStats(RetInvocable<void, Args const&...> auto const& test,
	              std::tuple<Args...> const& begins,
	              std::tuple<Args...> const& ends,
	              Modifiers<Args...> auto const& modifiers)
	{
		auto myadvance = [&](Args&... a) { return Advance<0>(std::tie(a...), begins, ends, modifiers); };
		GetStatsVectorizedAdvance(test, begins, myadvance);
	}
}

#include <iostream>
#include <fstream>
#include <random>
#include <filesystem>
#include <typeinfo>
#include <algorithm>

#include "opt-methods/math/Matrix.hpp"
#include "opt-methods/math/DenseMatrix.hpp"
#include "opt-methods/math/SkylineMatrix.hpp"
#include "opt-methods/math/Vector.hpp"

template<typename M>
concept ReadWritable = requires(M const& ct, std::filesystem::path const& p) {
	{ M::ReadFrom(p) } -> std::same_as<M>;
	ct.WriteTo(p);
};

namespace
{
	template<typename... Args>
	struct TypesTag
	{};
	template<typename... Args>
	constexpr inline TypesTag<Args...> typesTag;

	auto genDiag = []<SLESolver<double> M>(TypesTag<M>, int const& n, int const& k) -> M {
		std::vector<ptrdiff_t> diags;
		diags.reserve(2 * n - 2);
		for (int i = 1; i < n; i++)
		{
			diags.push_back(i);
			diags.push_back(-i);
		}
		return util::DiagonallyDominant(
				util::MatrixGenerator<double, M>(),
				n,
				pow(10, -k),
				diags,
				std::function<double(std::default_random_engine&)>(std::uniform_int_distribution<int>(-4, 0)));
	};

	auto genHilbert = []<SLESolver<double> M>(TypesTag<M>, int const& n) -> M {
		std::vector<ptrdiff_t> diags;
		diags.reserve(2 * n - 2);
		for (int i = 1; i < n; i++)
		{
			diags.push_back(i);
			diags.push_back(-i);
		}
		return util::Hilbert(
				util::MatrixGenerator<double, M>(),
				n,
				diags);
	};
}

template<typename T>
std::string GetClassDir()
{
	std::string className = typeid(T).name();
	className.erase(0, className.find(' ') + 1);
	std::replace(className.begin(), className.end(), ' ', '-');
	std::replace(className.begin(), className.end(), '<', '@');
	std::replace(className.begin(), className.end(), '>', '@');
	return className;
}

template<SLESolver<double>... Ms, typename... Args, typename Gen>
  requires ((ReadWritable<Ms> && ...) && (RetInvocable<Gen, Ms, TypesTag<Ms>, Args const&...> && ...))
auto GenTestsTest(std::filesystem::path const& parentDir, Gen const& gen,
	                std::vector<std::pair<std::tuple<Args...>, std::filesystem::path>>& res, TypesTag<Args...>)
{
	return [&](Args const&... args) {
		std::stringstream ss{};
		PrintTupleLike<'_'>(ss, std::make_tuple(args...));
		std::string testName = ss.str();
		std::filesystem::path testDir = parentDir / testName;
		res.emplace_back(std::make_tuple(args...), testDir);

		CompiletimeLoop<sizeof...(Ms)>::Go([&]<std::size_t i>(std::integral_constant<std::size_t, i>) {
			using M = std::tuple_element_t<i, std::tuple<Ms...>>;
			std::filesystem::path classDir = testDir / classToName(typesTag<M>);
			std::filesystem::create_directories(classDir);

			auto matrix = gen(typesTag<M>, args...);
			matrix.WriteTo(classDir);
		});
	};
}

template<SLESolver<double>... Ms, typename... Args, typename Gen>
  requires ((ReadWritable<Ms> && ...) && (RetInvocable<Gen, Ms, TypesTag<Ms>, Args const&...> && ...))
auto GenTestsVectorizedAdvance(std::filesystem::path const& parentDir, Gen const& gen,
	                             std::tuple<Args...> const& begins, RetInvocable<bool, Args&...> auto const& modifiers)
{
	std::vector<std::pair<std::tuple<Args...>, std::filesystem::path>> res;
	GetStatsVectorizedAdvance(
	    GenTestsTest<Ms...>(parentDir, gen, res, typesTag<Args...>),
		  begins,
		  modifiers);
	return res;
}

template<SLESolver<double>... Ms, typename... Args, typename Gen>
  requires ((ReadWritable<Ms> && ...) && (RetInvocable<Gen, Ms, TypesTag<Ms>, Args const&...> && ...))
auto GenTests(std::filesystem::path const& parentDir, Gen const& gen,
	            std::tuple<Args...> const& begins, std::tuple<Args...> const& ends,
	            Modifiers<Args...> auto const& modifiers)
{
	std::vector<std::pair<std::tuple<Args...>, std::filesystem::path>> res;
	GetStats(
		  GenTestsTest<Ms...>(parentDir, gen, res, typesTag<Args...>),
		  begins, ends,
		  modifiers);
	return res;
}

template<SLESolver<double>... Ms, typename... Args, typename TestFn>
  requires ((ReadWritable<Ms> && ...) && (std::invocable<TestFn, Ms&&, Args const&...> && ...))
auto RunTests(std::vector<std::pair<std::tuple<Args...>, std::filesystem::path>> const& tests,
	            TestFn const& testFn, TypesTag<Args...>)
{
	for (auto& [args, path] : tests)
		CompiletimeLoop<sizeof...(Ms)>::Go([&]<std::size_t i>(std::integral_constant<std::size_t, i>) {
			std::apply(testFn,
			           std::tuple_cat(std::make_tuple(std::tuple_element_t<i, std::tuple<Ms...>>::ReadFrom(
			                              path / classToName(typesTag<std::tuple_element_t<i, std::tuple<Ms...>>>))),
			                          args));
		});
}

template<SLESolver<double> M>
std::string classToName(TypesTag<M>)
{
	if constexpr (std::is_same_v<SkylineMatrix<double>, M>)
		return "LU";
	else
		return "Gauss";
};

template<SLESolver<double>... Ms, typename... Args, typename Gen, typename... TableArgs, typename TestFn>
  requires ((ReadWritable<Ms> && ...) &&
		        (RetInvocable<Gen, Ms, TypesTag<Ms>, Args const&...> && ...) &&
		        (RetInvocable<TestFn, std::tuple<TableArgs...>, Ms&&, Args const&...> && ...))
std::unordered_map<std::string, Table<TableArgs...>> Test(
        std::string const& testName,
        TypesTag<TableArgs...>,
        std::tuple<std::conditional_t<true, std::string, TableArgs>...> const& tableNames,
        Gen const& gen,
        std::tuple<Args...> const& begins,
        std::tuple<Args...> const& ends,
        Modifiers<Args...> auto const& modifiers,
        TestFn const& testFn)
{
	std::filesystem::path testDir = "doc/3/test";

	std::unordered_map<std::string, Table<TableArgs...>> tables;
	CompiletimeLoop<sizeof...(TableArgs)>::Go([&]<std::size_t i>(std::integral_constant<std::size_t, i>) {
		((tables[classToName(typesTag<Ms>)].names[i] = std::get<i>(tableNames)), ...);
	});
	auto res = GenTests<Ms...>(testDir / "in" / testName, gen, begins, ends, modifiers);
	RunTests<Ms...>(res,
	    [&]<SLESolver<double> M, typename... TArgs>(M&& A, TArgs const&... args)
		      requires(std::same_as<Args, TArgs> && ...) {
		    // TArgs for MSVC bug workaround
		    tables[classToName(typesTag<M>)].Add(testFn(std::move(A), args...));
	    },
	    typesTag<Args...>);

	auto outDir = testDir / "out" / testName;
	std::filesystem::create_directories(outDir);
	for (auto& [name, table] : tables)
	{
		std::ofstream o(outDir / (name + ".tsv"));
		o << table;
	}
	return tables;
}

int main()
{
	/*
	// examples:
	std::cout << " -= example 1 =-\n";
	auto test = [](int const& i, double const& j) {
	  std::cout << "i=" << i << "\tj=" << j << std::endl;
	};
	GetStats(std::function(test),
	    std::make_tuple(0, 1.0),
	    std::make_tuple(2, 16.0),
	    std::make_tuple(std::function([](int const& i) { return i + 1; }),
	      std::function([](double const& j) { return j * 2; })));
	std::cout << " -= example 2 =-\n";
	GetStatsVectorizedAdvance(std::function(test),
	    std::make_tuple(0, 1.0),
	    std::function([](int& i, double& e) {
	      if (i > 10)
	        return false;
	      i++;
	      e = std::exp(i);
	      return true;
	    }));
	std::cout << " -= table =-\n";
	auto table = Table<int, double, double>();
	table.names[0] = "Int!";
	table.names[1] = "Δ";
	table.names[2] = "ε";
	table.Add(0, 10, 1);
	table.Add(30, 12.3, 0.5);
	std::cout << table << std::flush;
	//*/

	auto testDiffTable = [&]<SLESolver<double> M>(M&& A, int n) {
		Vector<double> x_star(0.0, n);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		Vector<double> b = A * x_star, x = std::move(A).SolveSystem(b);
		return std::make_tuple(n,
		                       Len(static_cast<Vector<double>>(x_star - x)),
		                       sqrt(Len2(static_cast<Vector<double>>(x_star - x)) / Len2(x_star)));
	};

	auto testDiffTableK = [&]<SLESolver<double> M>(M&& A, int n, int k) {
		[[maybe_unused]] auto [n_, delta, eps] = testDiffTable(std::move(A), n);
		return std::make_tuple(n, k, delta, eps);
	};

	using namespace std::literals;
	Test<SkylineMatrix<double>, DenseMatrix<double>>(
	    "diag", typesTag<int, int, double, double>,
	    std::make_tuple("n"s, "k"s, "Δ"s, "ε"s),
	    genDiag,
	    std::make_tuple(10, 0),
	    std::make_tuple(1281, 1000),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 2); }, [](int const& k) -> int { return k + 300; }),
	    testDiffTableK);
	Test<SkylineMatrix<double>, DenseMatrix<double>>(
	    "hilbert", typesTag<int, double, double>,
	    std::make_tuple("n"s, "Δ"s, "ε"s),
	    genHilbert,
	    std::make_tuple(10),
	    std::make_tuple(1281),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 1.5); }),
	    testDiffTable);

	return 0;
}
