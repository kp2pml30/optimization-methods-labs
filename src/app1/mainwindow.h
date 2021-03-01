#pragma once

#include <QMainWindow>
#include <QPainter>
#include <QTimer>

#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/ErasedApproximator.hpp"
#include <QtCharts/QLineSeries>

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

private:
	using Approx = ErasedApproximator<double, double>;
	using Solver = IterationalSolver<double, double, Approx>;

	Solver approx;
	std::function<double (double)> func = [](double x) { return std::pow(x, 4) - 1.5 * atan(x); };
	RangeBounds<double> r = RangeBounds<double>(-1, 1);
	Solver::SolveData data;
	QtCharts::QLineSeries* plot = nullptr;

	std::unique_ptr<Ui::MainWindow> ui;
};
