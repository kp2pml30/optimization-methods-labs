
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

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	plot = new QtCharts::QLineSeries();
	{
		qreal l = -1, r = 1;
		auto func = [](qreal x) { return pow(x, 4) - 1.5 * atan(x); };
		for (int i = 0; i <= 100; i++)
		{
			qreal x = l + i / 100.0 * (r - l);
			QPointF p(x, func(x));
			*plot << p;
		}
	}

	auto ch = new QtCharts::QChart();
	// ch->setTheme(QtCharts::QChart::ChartThemeLight);
	ch->addSeries(plot);

	ch->createDefaultAxes();
	{
		auto* x = static_cast<QtCharts::QValueAxis*>(ch->axisX());
		x->setTickAnchor(0);
		x->setTickInterval(0.5);
		x->setTickType(QtCharts::QValueAxis::TicksDynamic);
		auto* y = static_cast<QtCharts::QValueAxis*>(ch->axisY());
		y->setTickAnchor(0);
		y->setTickInterval(0.5);
		y->setTickType(QtCharts::QValueAxis::TicksDynamic);
	}

	ui->setupUi(this); // this line gives dataraces even in empty solution >_<
	ui->graphicsView->setChart(ch);

	ch = new QtCharts::QChart();
	ch->setAnimationOptions(QtCharts::QChart::SeriesAnimations);
	ui->graphicsView_2->setChart(ch);

	for (auto&& [_, name] : factories)
		ui->comboBox->addItem(QString(name.c_str()));
}

void MainWindow::iterationNChanged(int n)
{
	assert(approx.has_value());

	auto ch = ui->graphicsView->chart();
	ch->removeSeries(plot);
	ch->removeAllSeries();
	ch->addSeries(plot);
	plot->attachAxis(ch->axisX());
	plot->attachAxis(ch->axisY());

	approx->approximator.draw(data[n].second, *data[n].first, *ch);
}

void MainWindow::recalc(int n, double eps)
{
	approx = factories[n].first(eps);
	data.clear();
	approx->solveUntilEnd(func, r, data);

	auto* s = new QtCharts::QScatterSeries();
	auto* ch = ui->graphicsView_2->chart();
	{
		int i = 0;
		for (auto&& [d, r] : data)
			*s << QPointF{float(++i), r.r.p - r.l.p};
	}
	ch->removeAllSeries();
	ch->addSeries(s);
	ch->createDefaultAxes();
	auto* x = static_cast<QtCharts::QValueAxis*>(ch->axisX());
	auto* y = static_cast<QtCharts::QValueAxis*>(ch->axisY());
	x->setTickAnchor(1);
	x->setTickInterval(1);
	x->setTickCount(data.size());
	// x->applyNiceNumbers();
	x->setLabelFormat("%i");
	qreal r = y->max() - y->min();
	y->setRange(y->min() - r / 100, y->max() + r / 100);
	y->setTickAnchor(0);
	y->setTickInterval(r / 10);
	y->setTickType(QtCharts::QValueAxis::TicksDynamic);

	ui->horizontalSlider->setMinimum(0);
	ui->horizontalSlider->setMaximum(data.size() - 1);
	ui->horizontalSlider->setDisabled(false);
	ui->horizontalSlider->setValue(0);
	iterationNChanged(0);
}

void MainWindow::methodChanged(int n) { recalc(n, ui->doubleSpinBox->value()); }

void MainWindow::epsChanged(double eps) { recalc(ui->comboBox->currentIndex(), eps); }

void MainWindow::paintEvent(QPaintEvent* ev)
{
	QMainWindow::paintEvent(ev);

	QPainter painter(this);
	ev->accept();

	update();
}

MainWindow::~MainWindow()
{}
