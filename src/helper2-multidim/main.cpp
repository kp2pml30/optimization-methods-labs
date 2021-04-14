#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/multidim/all.hpp"
#include "opt-methods/solvers/BaseApproximatorDraw.hpp"
#include "opt-methods/util/Charting.hpp"

#include <iostream>
#include <set>

namespace
{
	using Type    = double;
	using Mat     = DenseMatrix<Type>;
	using Vec     = Vector<Type>;
	using Bi2Func = QuadraticFunction<Type>;
	using PReg    = PointRegion<Vec>;

	constexpr std::size_t ITERATIONS_LIMIT = 100;

	struct Report
	{
		bool interrupted       = false;
		std::size_t iterations = std::numeric_limits<std::size_t>::max();
		PReg last{{0, 0}, -1};
	};

	struct Info
	{
		int dim  = -1;
		int cond = -1;
	};

	Report test1(Approximator<Vec, Type> auto& approx, Bi2Func const& func, PReg const& start)
	{
		Report result;
		std::size_t i = 0;
		auto gen      = approx(func, start);
		while (gen.next())
		{
			result.last = gen.getValue();
			if (++i >= ITERATIONS_LIMIT)
			{
				result.interrupted = true;
				break;
			}
		}
		result.iterations = i;
		return result;
	}

	template<typename T>
	concept OnedimProvider = requires(T const& t)
	{
		{
			t()
		}
		->Approximator<Type, Type>;
	};

	void test(OnedimProvider auto const& oneDimProvider,
	          Type eps,
	          Bi2Func const& func,
	          PReg const& start,
	          std::function<void(std::string const&, Report const&)> callback)
	{
		using Onedim = decltype(oneDimProvider());
		std::map<std::string, Report> result;
		{
			auto desc = GradientDescent<Vec, Type>(eps);
			callback(desc.name(), test1(desc, func, start));
		}
		{
			auto desc = SteepestDescent<Vec, Type, Onedim>(eps, oneDimProvider());
			callback(desc.name(), test1(desc, func, start));
		}
		{
			auto desc = ConjugateGradientDescent<Vec, Type, Bi2Func>(eps);
			callback(desc.name(), test1(desc, func, start));
		}
	}

	template<typename T>
	struct Table
	{
		// axis_name -> value
		std::set<T> x;
		std::string xName = "x";
		std::map<std::string, std::map<T, T>> cols;

		Table& add(T const& tx, std::string const& col, T const& val)
		{
			x.emplace(tx);
			cols[col][tx] = val;
			return *this;
		}

		Table& operator<<(std::pair<std::string, std::map<T, T>> w)
		{
			for (auto const& v : w.second)
				x.emplace(v.first);
			assert(cols.insert(std::move(w)).second);
			return *this;
		}

		friend std::ostream& operator<<(std::ostream& o, Table const& g)
		{
			o << g.xName;
			for (auto const& a : g.cols)
				o << '\t' << a.first;
			o << '\n';
			for (auto const& x : g.x)
			{
				o << x;
				for (auto const& a : g.cols)
				{
					o << '\t';
					if (auto iter = a.second.find(x); iter != a.second.end()) o << iter->second;
				}
				o << '\n';
			}
			return o;
		}
	};
} // namespace

int main()
{
	std::map<std::string, Table<int>> graphs;

	double eps = 1e-5;

	auto const& addResult = [&](std::string const& approxName, Info const& info, Report const& r) {
		if (r.interrupted)
		{
			std::cerr << "interrupted" << std::endl;
			abort();
		}
		graphs[approxName].xName = "cond";
		graphs[approxName].add(info.cond, "n=" + std::to_string(info.dim), r.iterations);
	};

	{
		Info info;
		info.dim           = 2;
		auto bifunc        = QuadraticFunction2d<Type>(1, 0, 1, 0, 0, -1);
		auto const& onedim = []() { return GoldenSectionApproximator<Type, Type>(1e-5); };
		test(onedim, eps, bifunc, PReg({1, 1}, 10), [&](std::string const& name, Report const& rep) {
			addResult(name, info, rep);
		});
	}

	{
		Table<std::string> paramsTable;
		paramsTable.xName = "property";
		paramsTable << std::make_pair<std::string, std::map<std::string, std::string>>("epsilon",
		                                                                               {{"value", std::to_string(eps)}});
		paramsTable << std::make_pair<std::string, std::map<std::string, std::string>>("start", {{"value", "(1;...1)"}});
		std::cout << "-== params ==-\n\n" << paramsTable;
	}

	for (auto const& g : graphs)
	{
		std::cout << "-== " << g.first << " ==-\n\n";
		std::cout << g.second << std::endl;
	}
	return 0;
}
