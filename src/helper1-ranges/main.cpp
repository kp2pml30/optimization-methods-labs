#include "opt-methods/approximators/Brent.hpp"
#include "opt-methods/approximators/Dichotomy.hpp"
#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/approximators/Fibonacci.hpp"
#include "opt-methods/approximators/Parabolic.hpp"

#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/ErasedApproximator.hpp"

#include <map>
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>
#include <fstream>
#include <algorithm>

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cerr << "expected output path prefix" << std::endl;
		return 1;
	}
	std::string prefix = argv[1];

	std::cout << std::setprecision(std::numeric_limits<double>::digits10 + 1);

	auto func = [](double x) { return std::pow(x, 4) - 1.5 * atan(x); };

	constexpr double EPSILON = 1e-7;

	auto approximators = IterationalSolverBuilder<double, double,
				DichotomyApproximator<double, double>,
				GoldenSectionApproximator<double, double>,
				FibonacciApproximator<double, double>,
				ParabolicApproximator<double, double>,
				BrentApproximator<double, double>
			>
		(
			std::make_tuple(EPSILON),
			std::make_tuple(EPSILON),
			std::make_tuple(EPSILON),
			std::make_tuple(EPSILON),
			std::make_tuple(EPSILON)
		);


	struct IterationInfo
	{
		int iterationsCount;
		double range;
	};

	std::map<double, std::map<std::string, IterationInfo>> iterations;

	std::ofstream cout;
	auto printHeader = [&](std::ostream& cout) {
		cout << "i\tleft\tright\tlog(ratio)\tlval\trval\n";
	};

	std::map<int, std::map<std::string, double>> ratios;

	std::set<std::string> names;

	auto walker =  [&](auto& approx, RangeBounds<double> const& r) {
		typename std::decay_t<decltype(approx)>::SolveData info;
		approx.solveDiff(func, EPSILON * 2.2, r, info);
		std::string name = approx.approximator.name();
		std::replace(name.begin(), name.end(), ' ', '-');
		names.emplace(name);

		cout = std::ofstream(prefix + "/" + name +"Range.csv");
		printHeader(cout);
		int c = 0;
		for (auto const& i : info)
		{
			double ratio = std::log((i.second.r.p - i.second.l.p) / (r.r - r.l));
			ratios[c][name] = ratio;
			cout
				<< c++
				<< '\t' << i.second.l.p << '\t' << i.second.r.p
				<< '\t' << ratio
				<< '\t' << func(i.second.l.p) << '\t' << func(i.second.r.p)
				<< '\n'
				;
		}
	};

	approximators.each(walker, RangeBounds<double>(-1, 1));


	cout = std::ofstream(prefix + "/ratios.csv");
	cout << 'i';
	for (auto const& i : names)
		cout << '\t' << i;
	cout << '\n';
	for (auto const& i : ratios)
	{
		cout << i.first;
		for (auto const& n : names)
		{
			cout << '\t';
			auto iter = i.second.find(n);
			if (iter != i.second.end())
				cout << iter->second;
			else
				cout << "nan";
		}
		cout << '\n';
	}

	return 0;
}
