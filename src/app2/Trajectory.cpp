#include "mainwindow.h"

using namespace QtCharts;

MainWindow::Trajectory::Trajectory()
{
}

QPolygonF MainWindow::Trajectory::createArrowHead(QPointF from, QPointF to)
{
	using namespace std::numbers;

	auto line = QLineF(from, to);
	auto length = line.length();

	qreal arrowSize = std::min(20.0, length / 10);

	double angle = acos(line.dx() / length);
	if (line.dy() >= 0)
		angle = pi * 2 - angle;

	double deltaAngle = pi / 2 - pi / 12;

	QPointF arrowP1 = line.p2() -
		QPointF(sin(angle +      deltaAngle) * arrowSize, cos(angle +      deltaAngle) * arrowSize);
	QPointF arrowP2 = line.p2() -
		QPointF(sin(angle + pi - deltaAngle) * arrowSize, cos(angle + pi - deltaAngle) * arrowSize);

	QPolygonF poly;
	return poly << line.p2() << arrowP1 << arrowP2;
}

void MainWindow::Trajectory::setName(const std::string &name)
{
	lines->setName(name.c_str());
}

namespace
{
	QFont defaultFont = QFont("", 6, QFont::Bold);
	QBrush defaultBrush(QColorConstants::Black);
	QPen nullPen(defaultBrush, 0);
}

void MainWindow::Trajectory::addPoint(Vector<double> p_)
{
	{
		[[maybe_unused]] static auto dummy = []() {
			nullPen.setWidth(0);
			return 0;
		}();
	}

	auto p = QPointF{p_[0], p_[1]};
	*lines << p;

	if (lastP.has_value())
	{
		auto scale = QLineF{*lastP, p}.length();
		auto arrow = new QGraphicsPolygonItem(createArrowHead(*lastP - p, QPointF{0, 0}));
		arrow->setPen(nullPen);
		arrow->setBrush(defaultBrush);
		arrows.push_back(new ChartItem(arrow, p, 300 / scale));
		auto label = new QGraphicsTextItem();
		label->setHtml(QString::fromStdString("<div style='background:rgba(255, 255, 255, 50%);'>x<sub>" +
																					std::to_string(arrows.size() / 2 + 1) + "</sub></div>"));
		label->setFont(defaultFont);
		label->setTransform(QTransform::fromScale(1, -1));
		label->setScale(scale / 150);
		arrows.push_back(new ChartItem(label, p, 300 / scale));
	}

	lastP = p;
}

void MainWindow::Trajectory::setColor(QColor color)
{
	lines->setColor(color);
}

void MainWindow::Trajectory::addLevel(QLineSeries *levelSet)
{
	levelSets.push_back(levelSet);
}

void MainWindow::Trajectory::setVisible(bool visible)
{
	lines->setVisible(visible);
	setVisibleArrowheads(arrowheadVisibility && visible);
	setVisibleLevelSets(levelSetsVisibility && visible);
}

bool MainWindow::Trajectory::isVisible() const
{
	return lines->isVisible();
}

void MainWindow::Trajectory::setVisibleArrowheads(bool visible)
{
	visible &= isVisible();
	for (auto* a : arrows)
		a->setVisible(visible);
}

void MainWindow::Trajectory::setVisibleLevelSets(bool visible)
{
	visible &= isVisible();
	for (auto* s : levelSets)
		s->setVisible(visible);
	if (visible)
		for (auto* s : levelSets)
			for (auto* a : chart->legend()->markers(s))
				a->setVisible(false);
}

void MainWindow::Trajectory::toggleArrowheads(bool visible)
{
	setVisibleArrowheads(arrowheadVisibility = visible);
}

void MainWindow::Trajectory::toggleLevelSets(bool visible)
{
	setVisibleLevelSets(levelSetsVisibility = visible);
}

void MainWindow::Trajectory::addToChart(QChart* chart)
{
	this->chart = chart;

	chart->addSeries(lines);
	for (auto* a : arrows)
		a->setColor(lines->color());
	for (auto* s : levelSets)
	{
		chart->addSeries(s);
		for (auto* a : chart->legend()->markers(s))
			a->setVisible(false);
	}
}

void MainWindow::Trajectory::addToChartView(NavigableChartView *view)
{
	this->view = view;

	for (auto* a : arrows)
		view->addItem(a);
}

MainWindow::Trajectory::~Trajectory()
{
	if (chart == nullptr)
	{
		// no one has taken ownerwhip of these items
		delete lines;
		for (auto* s : levelSets)
			delete s;
	}
	if (view == nullptr)
	{
		// no one has taken ownerwhip of these items
		for (auto* a : arrows)
			delete a;
	}
}
