#pragma once

#include <vector>

#include <QMainWindow>
#include <QtCharts/QLegendMarker>
#include <QGraphicsItem>

#include "opt-methods/solvers/Erased.hpp"
#include "opt-methods/math/BisquareFunction.hpp"
#include "opt-methods/solvers/IterationalSolver.hpp"

#include "opt-methods/approximators/all.hpp"
#include "opt-methods/solvers/BaseApproximatorDraw.hpp"

#include "NavigableChartView.h"

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

class QCheckBox;

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
	void screenshot();
	void toggleArrowheads(bool);
	void toggleLevelSets(bool);
	void startXChanged(double);
	void startYChanged(double);
	void functionChanged(int);

private:
	using Approx = ErasedApproximator<double, double>;
	using MApprox = ErasedApproximator<Vector<double>, double>;

	std::map<std::string, QCheckBox*> gradientTogglers;

	static inline std::string defaultTrajName = "Default level sets";

	const std::vector<QuadraticFunction2d<double>> bifuncs = {
		QuadraticFunction2d<double>(2.5, 3, 4, 0.1, -0.5, 10),
		QuadraticFunction2d<double>(8, 1, 1, 0, 0, -1),
		QuadraticFunction2d<double>(3, 2.5, 4, 0.1, -0.5, 0),
		QuadraticFunction2d<double>(1, 0, 1, -0.5, 0, 0),
		QuadraticFunction2d<double>(1, -1.2, 1, 0, 0, 0),
	};

	QtCharts::QChart* chart = nullptr;
	class Trajectory
	{
	private:
		std::vector<ChartItem*> arrows;
		QtCharts::QLineSeries* lines = new QtCharts::QLineSeries();
		std::vector<QtCharts::QLineSeries*> levelSets;

		bool arrowheadVisibility = true, levelSetsVisibility = true;
		std::optional<QPointF> lastP;
		QtCharts::QChart* chart = nullptr;
		NavigableChartView* view = nullptr;

		static QPolygonF createArrowHead(QPointF from, QPointF to);
		void setVisibleArrowheads(bool visible);
		void setVisibleLevelSets(bool visible);

	public:
		Trajectory();
		void setName(const std::string &name);
		void addPoint(Vector<double> p_);
		void setColor(QColor color);
		void addLevel(QtCharts::QLineSeries *levelSet);
		void setVisible(bool visible);
		bool isVisible() const;
		void toggleArrowheads(bool visible);
		void toggleLevelSets(bool visible);
		void addToChart(QtCharts::QChart* chart);
		void addToChartView(NavigableChartView *view);
		~Trajectory();
	};
	std::unordered_map<std::string, Trajectory> name2trajectory;

	using OneDimFactoryT = std::pair<std::function<Approx(double)>, std::string>;
	using MultiDimFactoryT = std::pair<std::function<MApprox(double)>, std::string>;

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

	void addVisual(QuadraticFunction2d<double> const& func, MApprox& approx, Vector<double> start,
								 std::vector<std::pair<double, std::string>>& pointZts, QColor color);

	static inline std::vector<OneDimFactoryT> factories = getFactories<DichotomyApproximator,
																																		 FibonacciSizeTApproximator,
																																		 GoldenSectionApproximator,
																																		 ParabolicApproximator,
																																		 BrentApproximator>();

	void recalc();

	std::unique_ptr<Ui::MainWindow> ui;
	bool isInit = false;
};
