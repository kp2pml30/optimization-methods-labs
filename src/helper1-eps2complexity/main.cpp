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

	int calculationsCount;

	auto func = [&calculationsCount](double x) {
		calculationsCount++;
		return std::pow(x, 4) - 1.5 * atan(x);
	};

	constexpr double EPSILON = 1e-7;

	auto approximators = IterationalSolverBuilder<double, double,
				DichotomyApproximator<double, double>,
				GoldenSectionApproximator<double, double>,
				FibonacciApproximator<double, double>,
				ParabolicApproximator<double, double>
			>
		(
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
	std::set<std::string> names;
	double epsilon = 0.5;

	auto walker =  [&](auto& approx, RangeBounds<double> const& r) {
		calculationsCount = 0;
		typename std::decay_t<decltype(approx)>::SolveData dummy;
		auto result = approx.solveDiff(func, epsilon, r, dummy);
		std::string name = approx.approximator.name();
		std::replace(name.begin(), name.end(), ' ', '_');
		auto& info = iterations[epsilon][name];
		info.iterationsCount = calculationsCount;
		info.range = result.r.p - result.l.p;
		names.emplace(name);
	};

	for (epsilon = 0.5; epsilon > EPSILON * 2.2; epsilon /= 2)
		approximators.each(walker, RangeBounds<double>(-1, 1));

	std::ofstream cout;

	auto printHeader = [&](std::ostream& cout) {
		cout << "epsilon";
		for (auto const& it : names)
			cout << '\t' << it;
		cout << '\n';
	};

	cout = std::ofstream(prefix + "/epsilonToComplexity.csv");
	printHeader(cout);
	for (auto const& it : iterations)
	{
		cout << std::log(it.first);
		for (auto const& b : it.second)
			cout << '\t' << b.second.iterationsCount;
		cout << '\n';
	}

#if 0
	cout = std::ofstream(prefix + "/range.dat");
	printHeader(cout);
	for (auto const& it : iterations)
	{
		cout << it.first;
		for (auto const& b : it.second)
			cout << '\t' << b.second.range;
		cout << '\n';
	}
#endif

	return 0;
}
