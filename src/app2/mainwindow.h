#pragma once

#include <vector>

#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QtCharts/QLineSeries>

#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/ErasedApproximator.hpp"
#include "opt-methods/approximators/Dichotomy.hpp"
#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/approximators/Fibonacci.hpp"
#include "opt-methods/approximators/Parabolic.hpp"
#include "opt-methods/approximators/Brent.hpp"
#include "opt-methods/math/BisquareFunction.hpp"

QT_BEGIN_NAMESPACE
namespace Ui
{
	class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	void paintEvent(QPaintEvent*) override;

public slots:
	void methodChanged(int);
	void epsChanged(double);
	void powChanged(int);

private:
	using Approx = ErasedApproximator<double, double>;
	using Solver = IterationalSolver<double, double, Approx>;

	std::optional<Solver> approx{};

	std::function<double (double)> func = [](double x) { return std::sqrt(1 - x * x); };
	RangeBounds<double> r = RangeBounds<double>(-10, 10);
	Solver::SolveData data;
	QtCharts::QLineSeries* plot = nullptr;
	double lastEps              = 0;
	int lastPow                 = 0;

	using SolverConstructorT = std::function<Solver(double)>;

	using FactoryT = std::pair<SolverConstructorT, std::string>;

	template<Approximator<double, double> Approximator, typename ... Args>
	static FactoryT getFactory(Args&& ... args) requires std::is_constructible_v<Solver, TypeTag<Approximator>, Args&&...>
	{
		return {[=](double eps) { return Solver(typeTag<Approximator>, eps, std::forward<Args>(args)...); },
						Approximator::name()};
	}

	template<template<typename, typename> typename... Approxs>
		requires (... && Approximator<Approxs<double, double>, double, double>)
	static std::vector<FactoryT> getFactories() {
		return {getFactory<Approxs<double, double>>()...};
	}

	template<typename P, typename V>
	using FibonacciSizeTApproximator = FibonacciApproximator<P, V>; // for MSVC to match template template-parameter

	static inline std::vector<FactoryT> factories = getFactories<DichotomyApproximator,
																															 FibonacciSizeTApproximator,
																															 GoldenSectionApproximator,
																															 ParabolicApproximator,
																															 BrentApproximator>();

	void recalc();

	std::unique_ptr<Ui::MainWindow> ui;
};
