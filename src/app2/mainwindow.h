#pragma once

#include <qchart.h>
#include <vector>

#include <QMainWindow>
#include <QPainter>
#include <QTimer>
#include <QtCharts/QLineSeries>

#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/Erased.hpp"
#include "opt-methods/approximators/Dichotomy.hpp"
#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/approximators/Fibonacci.hpp"
#include "opt-methods/approximators/Parabolic.hpp"
#include "opt-methods/approximators/Brent.hpp"
#include "opt-methods/multidim/GradientDescent.hpp"
#include "opt-methods/multidim/SteepestDescent.hpp"
#include "opt-methods/math/BisquareFunction.hpp"

QT_BEGIN_NAMESPACE
namespace Ui
{
	class MainWindow;
}
QT_END_NAMESPACE

template<template<typename, typename, typename> typename Base, template<typename, typename> typename Third>
struct BindMApprox
{
	using type = Base<Vector<double>, double, Third<double, double>>;
};

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();

	void paintEvent(QPaintEvent*) override;

public slots:
	void methodChanged(int);
	void multiMethodChanged(int);
	void epsChanged(double);
	void powChanged(int);

private:
	using Approx = ErasedApproximator<double, double>;
	using MApprox = ErasedApproximator<Vector<double>, double>;

	double lastEps = 0;
	int lastPow    = 0;

	BisquareFunction<double> bifunc = BisquareFunction<double>(8, 1, 1, 0, 0, -1);

	QtCharts::QChart* chart = nullptr;

	using OneDimFactoryT = std::pair<std::function<Approx(double)>, std::string>;

	template<Approximator<double, double> Approximator>
	static OneDimFactoryT getFactory()
	{
		return {[=](double eps) { return Approx(TypeTag<Approximator>{}, eps); }, Approximator::name()};
	}

	template<template<typename, typename> typename... Approxs>
	static std::vector<OneDimFactoryT> getFactories() {
		return { getFactory<Approxs<double, double>>()... };
	}

	template<typename P, typename V>
	using FibonacciSizeTApproximator = FibonacciApproximator<P, V>; // for MSVC to match template template-parameter

	void addVisual(MApprox& approx, std::vector<double>& pointZts);

	static inline std::vector<OneDimFactoryT> factories = getFactories<DichotomyApproximator,
																															 FibonacciSizeTApproximator,
																															 GoldenSectionApproximator,
																															 ParabolicApproximator,
																															 BrentApproximator>();

	void recalc();

	std::unique_ptr<Ui::MainWindow> ui;
};
