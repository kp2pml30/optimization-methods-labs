
#include "ui_mainwindow.h"
#include "mainwindow.h"

#include "opt-methods/approximators/Dichotomy.hpp"
#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/approximators/Fibonacci.hpp"
#include "opt-methods/approximators/Parabolic.hpp"
#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/ErasedApproximator.hpp"

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
	auto ch = new QtCharts::QChart();
	// auto bifunc = BisquareFunction<double>::Rand([]() { return rand() % 9 - 4; });
	auto bifunc = BisquareFunction<double>(2, 1, 1, 0, 0, -1);

	// TODO find minimum value and use it when drawing lines

	auto addLevel = [&](double delta, double colCoef) {
		auto copy = bifunc;
		copy.c -= delta;
		auto [f, t] = bifunc.zeroDescrYAt();
		if (std::isnan(f) || std::isinf(f))
			return;
		if (f > t)
			std::swap(f, t);
		auto bounds = RangeBounds<double>{f + 0.0001, t - 0.0001};
		auto series = Charting::plotCircular<QtCharts::QLineSeries>([&](auto const& x) { return copy.evalYPls(x); }, [&](auto const& x) { return copy.evalYNeg(x); }, bounds, 500, "");
		series->setColor(QColor((1 - colCoef) * 255, colCoef * 255, 0));
		ch->addSeries(series);
		for (auto *a : ch->legend()->markers(series))
			a->setVisible(false);
	};
	Vector<double> startFrom = {1, 1};
	double minVal = 0;
	double maxVal = std::numeric_limits<double>::quiet_NaN();
	{
		auto series = new QtCharts::QLineSeries();
		*series << QPointF{startFrom[0], startFrom[1]};
		GradientDescent<Vector<double>, double, DichotomyApproximator<double, double>> desc{0.001};
		auto gen = desc(bifunc, {startFrom, 0});
		while (gen.next())
		{
			auto res = gen.getValue();
			auto p = res.p;
			if (std::isnan(maxVal))
				maxVal = bifunc(res.p);
			minVal = bifunc(res.p);
			assert(p.size() == 2);
			*series << QPointF{p[0], p[1]};
		}
		series->setColor(QColor(0, 0, 255));
		series->setName(desc.name());
		ch->addSeries(series);
	}
	auto merp = [](auto l, auto r, auto c) {
		using std::pow;
		c = pow(c, 4);
		return (1 - c) * l + c * r;
	};
	constexpr int count = 30;
	for (int i = 0; i <= count; i++)
	{
		auto coef = i * 1.0 / count;
		addLevel(merp(minVal, maxVal, coef), coef);
	}
	ch->createDefaultAxes();
	Charting::growAxisRange(Charting::axisX<QtCharts::QValueAxis>(ch), 0.1);
	Charting::growAxisRange(Charting::axisY<QtCharts::QValueAxis>(ch), 0.1);

	ui->setupUi(this);
	ui->visualChartView->setChart(ch, 0.5);

	ui->function->setText(QString::fromStdString(static_cast<std::string>(bifunc)));

	ch = new QtCharts::QChart();
	ch->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
	ui->boundsChartView->setChart(ch);

	for (auto&& [_, name] : factories)
		ui->methodSelector->addItem(QString(name.c_str()));
}

void MainWindow::iterationNChanged(int n)
{
	assert(approx.has_value());

	auto ch = ui->visualChartView->chart();
	//ch->removeSeries(plot);
	//ch->removeAllSeries();
	//Charting::addToChart(ch, plot);

	// approx->approximator.draw(data[n].second, *data[n].first, *ch);
}

void MainWindow::recalc()
{
	int n = ui->methodSelector->currentIndex();
	double eps = ui->epsSelector->value() * pow(10, ui->powSelector->value());

	approx = factories[n].first(eps);
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

	ui->iterSelector->setMinimum(0);
	ui->iterSelector->setMaximum(static_cast<int>(data.size() - 1));
	ui->iterSelector->setDisabled(false);
	ui->iterSelector->setValue(0);

	iterationNChanged(0);
}

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
