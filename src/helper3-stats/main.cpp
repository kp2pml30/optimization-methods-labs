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
#include "opt-methods/math/RowColumnSymMatrix.hpp"
#include "opt-methods/math/Vector.hpp"
#include "opt-methods/math/CountedFloat.hpp"

#include "opt-methods/util/ScopeGuards.hpp"

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

	auto genDiag = []<typename T, SLESolver<T> M>(TypesTag<T, M>, int const& n, int const& k) -> M {
		std::vector<ptrdiff_t> diags;
		diags.reserve(2 * n - 2);
		for (int i = 1; i < n; i++)
		{
			diags.push_back(i);
			diags.push_back(-i);
		}
		return util::DiagonallyDominant(
		    util::MatrixGenerator<T, M>(),
		    n,
		    T{pow(10, -k)},
		    diags,
		    std::function<T(std::default_random_engine&)>(std::uniform_int_distribution<int>(-4, 0)));
	};

	auto genDiagSparse = []<typename T, SLESolver<T> M>(TypesTag<T, M>, int const& n) -> M {
		return util::DiagonallyDominant(
		    util::MatrixGenerator<T, M>(),
		    n,
		    T{1},
		    {-1, -2, -3, -4, 1, 2, 3, 4},
		    std::function<T(std::default_random_engine&)>(std::uniform_int_distribution<int>(-4, 0)));
	};

	auto genDiagRevSparse = []<typename T, SLESolver<T> M>(TypesTag<T, M>, int const& n) -> M {
		return util::DiagonallyDominant(
		    util::MatrixGenerator<T, M>(),
		    n,
		    T{1},
		    {-1, -2, -3, -4, 1, 2, 3, 4},
		    std::function<T(std::default_random_engine&)>(std::uniform_int_distribution<int>(0, 4)));
	};

	auto genHilbert = []<typename T, SLESolver<T> M>(TypesTag<T, M>, int const& n) -> M {
		std::vector<ptrdiff_t> diags;
		diags.reserve(2 * n - 2);
		for (int i = 1; i < n; i++)
		{
			diags.push_back(i);
			diags.push_back(-i);
		}
		return util::Hilbert(util::MatrixGenerator<T, M>(), n, diags);
	};
}

template<typename Gen, typename T, typename ArgsTuple, SLESolver<T>... Ms>
struct IsMsGenerator : std::false_type
{};

template<typename Gen, typename T, typename... Args, SLESolver<T>... Ms>
  requires (RetInvocable<Gen, Ms, TypesTag<T, Ms>, Args const&...> && ...)
struct IsMsGenerator<Gen, T, std::tuple<Args...>, Ms...> : std::true_type
{};

template<typename Gen, typename T, typename ArgsTuple, typename... Ms>
concept MsGenerator = IsMsGenerator<Gen, T, ArgsTuple, Ms...>::value;

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

template<typename T, SLESolver<T>... Ms, typename... Args, typename Gen>
  requires ((ReadWritable<Ms> && ...) && (RetInvocable<Gen, Ms, TypesTag<T, Ms>, Args const&...> && ...))
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
			std::filesystem::path classDir = testDir / classToName(typesTag<T, M>);
			std::filesystem::create_directories(classDir);

			auto matrix = gen(typesTag<T, M>, args...);
			matrix.WriteTo(classDir);
		});
	};
}

struct SimpleStat
{
public:
	static std::size_t ops;

	void operator()(char const*)
	{
		ops++;
	}
};
std::size_t SimpleStat::ops = 0;

template<typename T, SLESolver<T>... Ms, typename... Args, typename Gen>
  requires ((ReadWritable<Ms> && ...) && (RetInvocable<Gen, Ms, TypesTag<T, Ms>, Args const&...> && ...))
auto GenTestsVectorizedAdvance(std::filesystem::path const& parentDir, Gen const& gen,
	                             std::tuple<Args...> const& begins, RetInvocable<bool, Args&...> auto const& modifiers)
{
	std::vector<std::pair<std::tuple<Args...>, std::filesystem::path>> res;
	GetStatsVectorizedAdvance(
	    GenTestsTest<T, Ms...>(parentDir, gen, res, typesTag<Args...>),
		  begins,
		  modifiers);
	return res;
}

template<typename T, SLESolver<T>... Ms, typename... Args, typename Gen>
  requires ((ReadWritable<Ms> && ...) && (RetInvocable<Gen, Ms, TypesTag<T, Ms>, Args const&...> && ...))
auto GenTests(std::filesystem::path const& parentDir, Gen const& gen,
	            std::tuple<Args...> const& begins, std::tuple<Args...> const& ends,
	            Modifiers<Args...> auto const& modifiers)
{
	std::vector<std::pair<std::tuple<Args...>, std::filesystem::path>> res;
	GetStats(
		  GenTestsTest<T, Ms...>(parentDir, gen, res, typesTag<Args...>),
		  begins, ends,
		  modifiers);
	return res;
}

template<typename T, SLESolver<T>... Ms, typename... Args, typename TestFn>
  requires ((ReadWritable<Ms> && ...) && (std::invocable<TestFn, Ms&&, Args const&...> && ...))
auto RunTests(std::vector<std::pair<std::tuple<Args...>, std::filesystem::path>> const& tests,
	            TestFn const& testFn, TypesTag<Args...>)
{
	for (auto& [args, path] : tests)
		CompiletimeLoop<sizeof...(Ms)>::Go([&]<std::size_t i>(std::integral_constant<std::size_t, i>) {
			using M = std::tuple_element_t<i, std::tuple<Ms...>>;
			std::apply(testFn,
			           std::tuple_cat(std::make_tuple(M::ReadFrom(path / classToName(typesTag<T, M>))),
			                          args));
		});
}

template<typename T, SLESolver<T> M>
std::string classToName(TypesTag<T, M>)
{
	if constexpr (std::is_same_v<SkylineMatrix<T>, M>)
		return "LU";
	else if constexpr (std::is_same_v<DenseMatrix<T>, M>)
		return "Gauss";
	else
		return "ConjGrad";
};

template<typename... TableArgs>
void SaveTable(std::filesystem::path const& dir,
               std::unordered_map<std::string, Table<TableArgs...>> const& tables)
{
	std::filesystem::create_directories(dir);
	for (auto& [name, table] : tables)
	{
		std::ofstream o(dir / (name + ".tsv"));
		o << table;
	}
}

template<typename T, SLESolver<T>... Ms, typename... Args, typename Gen, typename... TableArgs, typename TestFn>
  requires ((ReadWritable<Ms> && ...) &&
		        (RetInvocable<Gen, Ms, TypesTag<T, Ms>, Args const&...> && ...) &&
		        (RetInvocable<TestFn, std::tuple<TableArgs...>, TypesTag<T>, Ms&&, Args const&...> && ...))
std::unordered_map<std::string, Table<TableArgs...>> Test(
        std::string const& testName,
        TypesTag<TableArgs...>,
        std::tuple<std::conditional_t<true, std::string, TableArgs>...> const& tableNames,
        Gen const& gen,
        std::tuple<Args...> const& begins,
        std::tuple<Args...> const& ends,
        Modifiers<Args...> auto const& modifiers,
        TestFn const& testFn, bool toSave = true)
{
	std::filesystem::path testDir = "doc/3/test";

	std::cerr << "testing " << testName << "..." << std::endl;
	SCOPE_GUARD_EX [start = std::chrono::system_clock::now()]() noexcept {
		using namespace std::chrono;
		std::cerr << " in " << duration_cast<milliseconds>(system_clock::now() - start).count() / 1000.0 << "s" << std::endl;
	};
	SCOPE_FAIL { std::cerr << "\tfail"; };
	SCOPE_SUCCESS { std::cerr << "\tsuccess"; };

	std::unordered_map<std::string, Table<TableArgs...>> tables;
	CompiletimeLoop<sizeof...(TableArgs)>::Go([&]<std::size_t i>(std::integral_constant<std::size_t, i>) {
		((tables[classToName(typesTag<T, Ms>)].names[i] = std::get<i>(tableNames)), ...);
	});
	auto res = GenTests<T, Ms...>(testDir / "in" / testName, gen, begins, ends, modifiers);
	RunTests<T, Ms...>(res,
		#if defined (_MSC_VER)
	    [&]<SLESolver<T> M, typename... TArgs>(M&& A, TArgs const&... args)
		    requires(std::same_as<Args, TArgs> && ...) {
		    // TArgs for MSVC bug workaround
		#else
	    [&]<SLESolver<T> M>(M&& A, Args const&... args) {
		#endif
		    tables[classToName(typesTag<T, M>)].Add(testFn(typesTag<T>, std::move(A), args...));
	    },
	    typesTag<Args...>);

	if (toSave)
	{
		std::cerr << "\tsaving " << testName << "..." << std::flush;
		SaveTable(testDir / "out" / testName, tables);
		std::cerr << " done" << std::endl;
	}
	return tables;
}

static int run()
{
	std::cout.precision(15);

	auto testDiffTable = []<typename T, SLESolver<T> M>(TypesTag<T>, M&& A, int n) {
		Vector<T> x_star(T{0.0}, n);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		Vector<T> b = A * x_star, x = std::move(A).SolveSystem(b);
		return std::make_tuple(n,
		                       Len(static_cast<Vector<T>>(x_star - x)),
		                       T{sqrt(Len2(static_cast<Vector<T>>(x_star - x)) / Len2(x_star))});
	};

	auto testDiffTableK = [&]<typename T, SLESolver<T> M>(TypesTag<T>, M&& A, int n, int k) {
		[[maybe_unused]] auto [n_, delta, eps] = testDiffTable(typesTag<T>, std::move(A), n);
		return std::make_tuple(n, k, delta, eps);
	};

	using namespace std::literals;

	Test<double, SkylineMatrix<double>, DenseMatrix<double>>(
	    "diag", typesTag<int, int, double, double>,
	    std::make_tuple("n"s, "k"s, "Δ"s, "ε"s),
	    genDiag,
	    std::make_tuple(10, 0),
	    std::make_tuple(1281, 1000),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 2); }, [](int const& k) -> int { return k + 300; }),
	    testDiffTableK);
	Test<double, SkylineMatrix<double>, DenseMatrix<double>>(
	    "hilbert", typesTag<int, double, double>,
	    std::make_tuple("n"s, "Δ"s, "ε"s),
	    genHilbert,
	    std::make_tuple(10),
	    std::make_tuple(1281),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 1.5); }),
	    testDiffTable);


	auto testDiffTableCF = []<typename T, SLESolver<T> M>(TypesTag<T>, M&& A, int n) {
		SimpleStat::ops = 0;
		Vector<T> x_star(T{0.0}, n);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		Vector<T> b = A * x_star, x = std::move(A).SolveSystem(b);
		return std::make_tuple(n, SimpleStat::ops);
	};

	using CF = CountedFloat<double, SimpleStat>;
	Test<CF, SkylineMatrix<CF>, DenseMatrix<CF>, RowColumnSymMatrix<CF>>(
	    "complexity_hilbert", typesTag<int, std::size_t>,
	    std::make_tuple("n"s, "i"s),
	    genHilbert,
	    std::make_tuple(10),
	    std::make_tuple(1281),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 1.5); }),
	    testDiffTableCF);
	Test<CF, SkylineMatrix<CF>, DenseMatrix<CF>, RowColumnSymMatrix<CF>>(
	    "complexity_sparse", typesTag<int, std::size_t>,
	    std::make_tuple("n"s, "i"s),
	    genDiagSparse,
	    std::make_tuple(10),
	    std::make_tuple(1281),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 1.5); }),
	    testDiffTableCF);

	auto testConjTable = [&](TypesTag<double>, RowColumnSymMatrix<double>&& A, int n) {
		Vector<double> x_star(0.0, n);
		std::iota(std::begin(x_star), std::end(x_star), 1);
		int nIters       = 0;
		Vector<double> b = A * x_star, x = A.SolveSystem(b, 1e-7, &nIters);
		auto [delta, eps] = std::make_tuple(Len(static_cast<Vector<double>>(x_star - x)),
		                                    sqrt(Len2(static_cast<Vector<double>>(x_star - x)) / Len2(x_star)));
		return std::make_tuple(n, nIters, delta, eps, eps * sqrt(Len2(b) / Len2(static_cast<Vector<double>>(b - A * x))));
	};

	Test<double, RowColumnSymMatrix<double>>(
	    "conj_diag",
	    typesTag<int, int, double, double, double>,
	    std::make_tuple("n"s, "iters"s, "Δ"s, "ε"s, "cond(A)"s),
	    genDiagSparse,
	    std::make_tuple(10),
	    std::make_tuple(100'000),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 1.3); }),
	    testConjTable);
	Test<double, RowColumnSymMatrix<double>>(
	    "conj_diag_rev",
	    typesTag<int, int, double, double, double>,
	    std::make_tuple("n"s, "iters"s, "Δ"s, "ε"s, "cond(A)"s),
	    genDiagRevSparse,
	    std::make_tuple(10),
	    std::make_tuple(100'000),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 1.3); }),
	    testConjTable);

	Test<double, RowColumnSymMatrix<double>>(
	    "conj_hilbert",
	    typesTag<int, int, double, double, double>,
	    std::make_tuple("n"s, "iters"s, "Δ"s, "ε"s, "cond(A)"s),
	    genHilbert,
	    std::make_tuple(10),
	    std::make_tuple(1'000),
	    std::make_tuple([](int const& n) -> int { return (int)(n * 1.2); }),
	    testConjTable);

	using namespace std::placeholders;
	Test<double, SkylineMatrix<double>, DenseMatrix<double>>(
	  "diagSkak", typesTag<int, double, double>,
	  std::make_tuple("n"s, "Δ"s, "ε"s),
	  std::bind(genDiag, _1, _2, 0),
	  std::make_tuple(10),
	  std::make_tuple(1000),
	  std::make_tuple([](int const& n) -> int { return n + 10; }),
	  testDiffTable);

	return 0;
}

int main()
{
	// workaround for scope guards (no stack unwinding without catches)
	try
	{
		return run();
	}
	catch (...)
	{
		throw;
	}
}
