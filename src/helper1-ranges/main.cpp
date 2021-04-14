#include "opt-methods/approximators/all.hpp"

#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/Erased.hpp"

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
		cout << "i,left,right,log(ratio),lval,rval\n";
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
			double ratio = std::log(2 * i.second.r / (r.r - r.l));
			ratios[c][name] = ratio;
			cout
				<< c++
				<< ',' << i.second.p - i.second.r << ',' << i.second.p + i.second.r
				<< ',' << ratio
				<< ',' << func(i.second.p - i.second.r) << ',' << func(i.second.p + i.second.r)
				<< '\n'
				;
		}
	};

	approximators.each(walker, RangeBounds<double>(-1, 1));

	cout = std::ofstream(prefix + "/ratios.tsv");
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
