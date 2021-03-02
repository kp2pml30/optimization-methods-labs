
#include "ui_mainwindow.h"
#include "mainwindow.h"

#include "opt-methods/approximators/Dichotomy.hpp"
#include "opt-methods/approximators/GoldenSection.hpp"
#include "opt-methods/approximators/Fibonacci.hpp"
#include "opt-methods/approximators/Parabolic.hpp"
#include "opt-methods/solvers/IterationalSolver.hpp"
#include "opt-methods/solvers/ErasedApproximator.hpp"

#include <QMouseEvent>
#include <QTimer>
#include <QtCharts/QValueAxis>

#include <cmath>
#include <iostream>
#include <iomanip>
#include <numbers>
#include <iostream>
#include <qnamespace.h>

#include "opt-methods/util/Charting.hpp"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	auto ch = new QtCharts::QChart();
	ch->addSeries(plot = Charting::plotFunction<QtCharts::QLineSeries>(func, r, 100, "f(x)"));
	ch->createDefaultAxes();
	Charting::growAxisRange(Charting::axisX<QtCharts::QValueAxis>(ch), 0.1);
	Charting::growAxisRange(Charting::axisY<QtCharts::QValueAxis>(ch), 0.1);

	ui->setupUi(this); // this line gives dataraces even in empty solution >_<
	ui->visualChartView->setChart(ch, 0.5);

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
	ch->removeSeries(plot);
	ch->removeAllSeries();
	Charting::addToChart(ch, plot);

	approx->approximator.draw(data[n].second, *data[n].first, *ch);
}

void MainWindow::recalc(int n, double eps)
{
	approx = factories[n].first(eps);
	data.clear();
	approx->solveUntilEnd(func, r, data);

	auto* ch = ui->boundsChartView->chart();
	ch->removeAllSeries();
	ch->addSeries(Charting::plotFunction<QtCharts::QScatterSeries>(
			[&](std::size_t i) { return data[i].second.r.p - data[i].second.l.p; },
			RangeBounds<std::size_t>(0, data.size() - 1),
			data.size(),
			"Search bound size on each iteration"));
	Charting::createNaturalSequenceAxes(ch, static_cast<int>(data.size()));

	ui->iterSelector->setMinimum(0);
	ui->iterSelector->setMaximum(static_cast<int>(data.size() - 1));
	ui->iterSelector->setDisabled(false);
	ui->iterSelector->setValue(0);

	ui->epsSelector->setMinimum(1e-10);

	iterationNChanged(0);
}

void MainWindow::methodChanged(int n) { recalc(n, ui->epsSelector->value()); }

void MainWindow::epsChanged(double eps)
{
	lastEps = eps;
}

void MainWindow::epsEditingFinished()
{
	recalc(ui->methodSelector->currentIndex(), lastEps);
}

void MainWindow::paintEvent(QPaintEvent* ev)
{
	QMainWindow::paintEvent(ev);

	QPainter painter(this);
	ev->accept();

	update();
}

MainWindow::~MainWindow()
{}
