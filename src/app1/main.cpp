#include "mainwindow.h"

#include "opt-methods/approximators/Dichotomy.hpp"
#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/ErasedApproximator.hpp"

#include <cmath>
#include <iostream>
#include <iomanip>
#include <QApplication>

int main(int argc, char* argv[])
{
	std::cout << std::setprecision(std::numeric_limits<double>::digits10 + 1);

	auto func = [](double x) { return std::pow(x, 4) - 1.5 * atan(x); };

	auto approximators = IterationalSolverBuilder<double, double,
				DichotomyApproximtor<double, double>,
				ErasedApproximator<double, double>
			>
		(
			std::make_tuple(1e-5),
			std::make_tuple(typeTag<DichotomyApproximtor<double, double>>, 1e-2)
		);

	auto walker =  [&](auto& approx, RangeBounds<double> const& r) {
		auto result = approx.solveIteration(func, 20, r);
		std::cout
			<< approx.approximator.name() << "\n"
			<< "\tl\t" << result.l.p << "\t" << result.l.v << "\n"
			<< "\tr\t" << result.r.p << "\t" << result.r.v
			<< std::endl
			;
	};
	approximators.each(walker, RangeBounds<double>(-1, 1));

	return 0;

	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	return a.exec();
}

