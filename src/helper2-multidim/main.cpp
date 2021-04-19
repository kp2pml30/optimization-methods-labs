#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/multidim/all.hpp"
#include "opt-methods/solvers/BaseApproximatorDraw.hpp"
#include "opt-methods/util/Charting.hpp"

#include <fstream>
#include <iostream>
#include <set>
#include <algorithm>
#include <functional>
#include <numeric>
#include <random>
#include <thread>
#include <mutex>
#include <filesystem>

namespace
{
	using Type    = double;
	using Mat     = DiagonalMatrix<Type>;
	using Vec     = Vector<Type>;
	using Bi2Func = QuadraticFunction<Type, Mat>;
	using PReg    = PointRegion<Vec>;

	constexpr std::size_t ITERATIONS_LIMIT = 50'000;

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
		{ t() } -> Approximator<Type, Type>;
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
		std::set<T> x;
		std::string xName = "x";
		std::map<std::string, std::map<T, T>> cols;

		Table& add(T const& tx, std::string const& col, T const& val)
		{
			x.emplace(tx);
			cols[col][tx] = val;
			return *this;
		}

		Table& merge(Table&& other)
		{
			xName = other.xName;
			x.merge(other.x);
			for (auto&& [str, map] : other.cols)
				cols[str].merge(map);
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

template<typename Task>
struct TaskQueue
{
	std::vector<Task> tasks;
	int nTasks, nCompleted = 0, percentage = 0;
	std::mutex m;

	TaskQueue(std::vector<Task> tasks, int nThreads)
	: tasks(std::move(tasks)), nTasks((int)this->tasks.size())
	, nCompleted(-nThreads)
	{}

	std::optional<Task> get()
	{
		std::unique_lock lg(m);

		++nCompleted;
		int newPercentage = nCompleted * 100 / nTasks;
		if (newPercentage > percentage) {
			std::cout << "\r" << (percentage = newPercentage) << "% complete";
		}

		if (tasks.empty()) return {};
		auto task = std::move(tasks.back());
		tasks.pop_back();
		return task;
	}
};

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr << "expected single argument: <path>" << std::endl;
		return 1;
	}
	std::string prefix = argv[1];

	int nThreads = std::thread::hardware_concurrency();
	std::vector<std::map<std::string, Table<int>>> graphsStorage(nThreads);
	std::map<std::string, Table<int>> graphs;

	double eps = 1e-5;

	{
		Table<std::string> paramsTable;
		paramsTable.xName = "property";
		paramsTable.add("value", "epsilon", std::to_string(eps)).add("value", "start", "(1;...;1)");
		auto prop = std::ofstream(prefix + "/properties.tsv");
		prop << paramsTable;
		std::cout << paramsTable << std::flush;
		for (auto const& a : paramsTable.cols)
			std::cout << a.first << std::endl;
	}

	auto const& saveToFiles = [&](std::string dir) {
		for (auto const& g : graphs)
		{
			namespace fs = std::filesystem;

			auto name = g.first;
			std::replace(name.begin(), name.end(), ' ', '-');
			auto dirp = fs::path(prefix)/dir;
			fs::create_directories(dirp);
			auto prop = std::ofstream(dirp/(name + ".tsv"));
			prop << g.second;
		}
	};

	auto engine = std::default_random_engine();

	auto testTable = [&](int thread_id, TaskQueue<std::pair<int, int>>& tasks, auto addResult) {
		while (true)
		{
			auto task = tasks.get();
			if (!task.has_value())
				return;

			auto [n, k] = *task;

			auto distrK = std::uniform_real_distribution<Type>(1, k);

			Info info;
			info.dim  = n;
			info.cond = k;

			std::valarray<Type> diag(n);
			diag[0] = 1;
			diag[1] = k;
			std::generate(std::next(std::begin(diag), 2), std::end(diag), [&]() { return distrK(engine); });
			std::sort(std::begin(diag), std::end(diag));

			auto bifunc        = Bi2Func(Mat(diag), Vec(0.0, n), 0.0);
			auto const& onedim = []() { return GoldenSectionApproximator<Type, Type>(1e-5); };
			test(onedim, eps, bifunc, PReg(Vec(1.0, n), 2.0 / (1 + k)), [&](std::string const& name, Report const& rep) {
				addResult(thread_id, name, info, rep);
			});
		}
	};

	auto parallelTestTable = [&](TaskQueue<std::pair<int, int>>& tasks, std::string tag, auto &addResult) {
		std::vector<std::thread> factory;
		for (int i = 0; i < nThreads; i++)
			factory.push_back(std::thread(testTable, i, std::ref(tasks), std::ref(addResult)));
		for (int i = 0; i < nThreads; i++)
			factory[i].join();
		for (auto& storage_block : graphsStorage)
			for (auto&& [name, table] : storage_block)
				graphs[name].merge(std::move(table));
		saveToFiles(tag);
	};

	{
		std::cout << "cond table\n";
		auto const& addResult = [&](int storage_id, std::string const& approxName, Info const& info, Report const& r) {
			if (r.interrupted)
			{
				std::cerr << "interrupted" << std::endl;
				abort();
			}
			graphsStorage[storage_id][approxName].xName = "cond";
			graphsStorage[storage_id][approxName].add(info.cond, "n=" + std::to_string(info.dim), (int)r.iterations);
		};

		const std::vector<int> ns = {10, 100, 1'000, 10'000};
		constexpr int kBegin = 1, kEnd = 2001, kStride = 100;
		constexpr int kN = (kEnd - kBegin) / kStride;

		std::vector<std::pair<int, int>> tasksStg;
		for (int k = kBegin; k <= kEnd; k += kStride)
			for (int n : ns)
				tasksStg.push_back({n, k});
		TaskQueue<std::pair<int, int>> tasks(tasksStg, nThreads);
		parallelTestTable(tasks, "cond", addResult);
	}

#if 0
	graphs.clear();
	for (auto &el : graphsStorage)
		el.clear();
	std::cout << '\n';

	{
		std::cout << "dims table\n";
		auto const& addResult = [&](int storage_id, std::string const& approxName, Info const& info, Report const& r) {
			if (r.interrupted)
			{
				std::cerr << "interrupted" << std::endl;
				abort();
			}
			graphsStorage[storage_id][approxName].xName = "dims";
			graphsStorage[storage_id][approxName].add(info.dim, "k=" + std::to_string(info.cond), (int)r.iterations);
		};

		const std::vector<int> ks = {10, 60, 360, 2160};
		constexpr int nBegin = 2, nEnd = 500, nStride = 1;
		constexpr int nN = (nEnd - nBegin) / nStride;

		std::vector<std::pair<int, int>> tasksStg;
		for (int k : ks)
			for (int n = nBegin; n <= nEnd; n += nStride)
				tasksStg.push_back({n, k});
		TaskQueue<std::pair<int, int>> tasks(tasksStg, nThreads);
		parallelTestTable(tasks, "dims", addResult);
	}
#endif /* 0 */

	return 0;
}
