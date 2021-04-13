
#include "ui_mainwindow.h"
#include "mainwindow.h"

#include "opt-methods/approximators/Dichotomy.hpp"
#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/approximators/Fibonacci.hpp"
#include "opt-methods/approximators/Parabolic.hpp"
#include "opt-methods/solvers/IterationalSolver.hpp"

#include "opt-methods/multidim/GradientDescent.hpp"

#include <QMouseEvent>
#include <QTimer>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLegendMarker>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <numbers>
#include <iostream>
#include <qnamespace.h>

#include "opt-methods/util/Charting.hpp"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->function->setText(QString::fromStdString(static_cast<std::string>(bifunc)));

	for (auto const& a : factories)
		ui->methodSelector->addItem(QString::fromStdString(a.second));
	recalc();
	recalc();
	recalc();
}

void MainWindow::addVisual(MApprox& desc, std::vector<double>& ys)
{
	Vector<double> startFrom = {1, 1};
	auto series = new QtCharts::QLineSeries();
	*series << QPointF{startFrom[0], startFrom[1]};
	ys.push_back(bifunc(startFrom));
	auto gen = desc(bifunc, {startFrom, 10});
	while (gen.next())
	{
		auto res = gen.getValue();
		auto p = res.p;
		ys.push_back(bifunc(p));
		assert(p.size() == 2);
		*series << QPointF{p[0], p[1]};
	}
	series->setName(desc.name());
	chart->addSeries(series);
}

void MainWindow::recalc()
{
	struct Smth {};
	const int onedimIndex = ui->methodSelector->currentIndex();
	double eps = ui->epsSelector->value() * pow(10, ui->powSelector->value());

	auto oldFlusher = std::unique_ptr<QtCharts::QChart>(chart);
	chart = new QtCharts::QChart();

	auto erasedProvider = [&]() { return factories[onedimIndex].first(eps); };
	std::vector<double> levels;
	{
		auto desc = MApprox(TypeTag<GradientDescent<Vector<double>, double>>{}, eps);
		addVisual(desc, levels);
	}
	{
		auto desc = MApprox(TypeTag<SteepestDescent<Vector<double>, double, ErasedApproximator<double, double>>>{}, eps, erasedProvider());
		addVisual(desc, levels);
	}

	auto addLevel = [&](double delta, double colCoef) {
		auto copy = bifunc.shift(-delta);
		auto [f, t] = bifunc.zeroDescrYAt();
		if (std::isnan(f) || std::isinf(f))
			return;
		if (f > t)
			std::swap(f, t);
		auto bounds = RangeBounds<double>{f, t};
		auto series = Charting::plotCircular<QtCharts::QLineSeries>(
				[&](auto const& x) { return copy.evalYPls(x); },
				[&](auto const& x) { return copy.evalYNeg(x); },
				bounds,
				500,
				"");
		series->setColor(QColor((1 - colCoef) * 255, colCoef * 255, 0));
		chart->addSeries(series);
		for (auto *a : chart->legend()->markers(series))
			a->setVisible(false);
	};

	if (!levels.empty())
	{
		std::sort(levels.begin(), levels.end());
		levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
		/// TODO change color distribution
		for (auto const& a : levels)
			addLevel(a, (a - levels.front()) / (levels.back() - levels.front()));
	}
	
	chart->createDefaultAxes();
	Charting::growAxisRange(Charting::axisX<QtCharts::QValueAxis>(chart), 0.1);
	Charting::growAxisRange(Charting::axisY<QtCharts::QValueAxis>(chart), 0.1);
	ui->visualChartView->setChart(chart, 0.5);
#if 0
	data.clear();
	approx->solveUntilEnd(func, r, data);

	auto* ch = ui->boundsChartView->chart();
	ch->removeAllSeries();
	ch->addSeries(Charting::plotFunction<QtCharts::QScatterSeries>(
			[&](std::size_t i) { return std::log(2 * data[i].second.r); },
			RangeBounds<std::size_t>(0, data.size() - 1),
			data.size(),
			"Search bound size log on each iteration"));
	Charting::createNaturalSequenceAxes(ch, static_cast<int>(data.size()));
	Charting::axisX(ch)->setTitleText("Number of iterations");
	Charting::axisY(ch)->setTitleText("log of search bound");
#endif
}

void MainWindow::multiMethodChanged(int) { recalc(); }
void MainWindow::methodChanged(int) { recalc(); }
void MainWindow::epsChanged(double) { recalc(); }
void MainWindow::powChanged(int) { recalc(); }

void MainWindow::paintEvent(QPaintEvent* ev)
{
	QMainWindow::paintEvent(ev);

	QPainter painter(this);
	ev->accept();

	update();
}

MainWindow::~MainWindow()
{}
