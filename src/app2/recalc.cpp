#include "ui_mainwindow.h"
#include "./mainwindow.h"

#include "opt-methods/multidim/all.hpp"
#include "opt-methods/util/Charting.hpp"

#include <QCheckBox>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QLegendMarker>
#include <QtCharts/QValueAxis>

#include <iostream>
#include <numbers>

using namespace QtCharts;

void MainWindow::addVisual(MApprox& desc, Vector<double> start, std::vector<std::pair<double, std::string>>& ys)
{
	auto &traj = name2trajectory[desc.name()];

	ys.push_back({bifunc(start), desc.name()});
	traj.addPoint(start);
	auto gen = desc(bifunc, {start, 10});
	int i = 0;
	while (gen.next())
	{
		if (i++ >= 1000)
		{
			std::cerr << desc.name() << "interrupted on " << i << " iteration" << std::endl;
			break;
		}
		auto res = gen.getValue();
		auto p = res.p;
		ys.push_back({bifunc(p), desc.name()});
		assert(p.size() == 2);
		traj.addPoint(p);
	}
	traj.setName(desc.name());
}

void MainWindow::recalc()
{
	name2trajectory.clear();

	struct Smth {};
	const int onedimIndex = ui->methodSelector->currentIndex();
	double eps = ui->epsSelector->value() * pow(10, ui->powSelector->value());
	Vector<double> start = {ui->startXSelector->value(), ui->startYSelector->value()};

	std::optional<QRectF> oldChartScreen;

	if (chart != nullptr) {
		auto *x = Charting::axisX<QValueAxis>(chart), *y = Charting::axisY<QValueAxis>(chart);
		oldChartScreen = QRectF{x->min(), y->min(), Charting::getAxisRange(x), Charting::getAxisRange(y)};
	}

	chart = new QChart();

	auto erasedProvider = [&]() { return factories[onedimIndex].first(eps); };
	std::vector<std::pair<double, std::string>> levels;
	{
		auto desc = MApprox(TypeTag<GradientDescent<Vector<double>, double>>{}, eps);
		addVisual(desc, start, levels);
	}
	{
		auto desc = MApprox(TypeTag<SteepestDescent<Vector<double>, double, ErasedApproximator<double, double>>>{}, eps, erasedProvider());
		addVisual(desc, start, levels);
	}
	{
		auto desc = MApprox(TypeTag<ConjugateGradientDescent<Vector<double>, double, QuadraticFunction2d<double>>>{}, eps);
		addVisual(desc, start, levels);
	}

	auto addLevel = [&](Trajectory& traj, double delta, double colCoef) {
		auto copy = bifunc.shift(-delta);
		auto [center, vx, vy] = copy.canonicalCoordSys();
		auto series = Charting::plotParametric<QSplineSeries>(
			[&](auto t) {
				return center + vx * cos(t) + vy * sin(t);
			},
			RangeBounds<double>{0.0, 2 * std::numbers::pi},
			20,
			"");
		// series->setColor();
		auto pen = series->pen();
		pen.setWidth(2);
		pen.setBrush(QBrush(QColor((1 - colCoef) * 255, colCoef * 255, 0)));
		series->setPen(pen);
		traj.addLevel(series);
	};

	if (!levels.empty())
	{
		std::sort(levels.begin(), levels.end());
		levels.erase(std::unique(levels.begin(), levels.end()), levels.end());
		/// TODO change color distribution
		double index = 0;
		for (auto const& [a, str] : levels)
			addLevel(name2trajectory[str], a, std::pow(index++ / (levels.size() - 1), 4));
	}

	{
		auto& defLevelSets = name2trajectory[defaultTrajName];
		defLevelSets.setName(defaultTrajName);
		for (int i = 0; i <= 100; i++)
			addLevel(defLevelSets, i, i / 100.0);
		for (auto& [str, t] : name2trajectory)
			t.addToChart(chart);
		defLevelSets.setVisible(false);
	}

	// add function to legend
	{
		auto add = new QLineSeries();
		add->setName(QString::fromStdString((std::string)bifunc));
		add->setColor(QColor(255, 0, 0));
		chart->addSeries(add);
	}
	chart->createDefaultAxes();
	if (oldChartScreen.has_value())
	{
		auto *x = Charting::axisX<QValueAxis>(chart), *y = Charting::axisY<QValueAxis>(chart);
		x->setRange(oldChartScreen->left(), oldChartScreen->right());
		y->setRange(oldChartScreen->top(), oldChartScreen->bottom());
	}
	ui->visualChartView->setChart(chart);
	for (auto &[str, t] : name2trajectory)
		t.addToChartView(ui->visualChartView);

	toggleArrowheads(ui->arrowheadsCheckBox->isChecked());
	toggleLevelSets(ui->levelSetsCheckBox->isChecked());
}
