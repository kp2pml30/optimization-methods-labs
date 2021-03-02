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
	void iterationNChanged(int);
	void methodChanged(int);
	void epsChanged(double);

private:
	using Approx = ErasedApproximator<double, double>;
	using Solver = IterationalSolver<double, double, Approx>;

	std::optional<Solver> approx{};

	std::function<double (double)> func = [](double x) { return std::pow(x, 4) - 1.5 * atan(x); };
	RangeBounds<double> r = RangeBounds<double>(-1, 1);
	Solver::SolveData data;
	QtCharts::QLineSeries* plot = nullptr;

	using SolverConstructorT = std::function<Solver(double)>;

	using FactoryT = std::pair<SolverConstructorT, std::string>;

	template<Approximator<double, double> Approximator, typename ... Args>
	static FactoryT getFactory(Args&& ... args) requires std::is_constructible_v<Solver, TypeTag<Approximator>, Args&&...>
	{
		return {[=](double eps) { return Solver(typeTag<Approximator>, eps, std::forward<Args>(args)...); },
						Approximator::name()};
	}

	static inline std::vector<FactoryT> factories = {
		getFactory<DichotomyApproximator<double, double>>(),
		getFactory<FibonacciApproximator<double, double, uint64_t>>(),
		getFactory<GoldenSectionApproximator<double, double>>(),
		getFactory<ParabolicApproximator<double, double>>()
	};

	void recalc(int n, double eps);

	std::unique_ptr<Ui::MainWindow> ui;
};
