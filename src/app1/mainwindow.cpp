
#include "ui_mainwindow.h"
#include "mainwindow.h"

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
#include "opt-methods/solvers/BaseApproximatorDraw.hpp"

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

	auto region2bounds = [&](PointRegion<double> pr) -> BoundsWithValues<double, double> {
		return {{pr.p - pr.r, func(pr.p - pr.r)}, {pr.p + pr.r, func(pr.p + pr.r)}};
	};

	approx->approximator.draw(region2bounds(data[n].second), *data[n].first, *ch);
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
