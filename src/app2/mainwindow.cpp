#include "ui_mainwindow.h"
#include "mainwindow.h"

#include <QMouseEvent>
#include <QTimer>
#include <QPainter>
#include <QCheckBox>

#include <limits>
#include <numbers>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	for (auto const& a : factories)
		ui->methodSelector->addItem(QString::fromStdString(a.second));
	auto const& addCheckbox = [&](std::string name) {
		auto box = new QCheckBox(this);
		box->setText(name.c_str());
		box->setChecked(true);
		connect(box, &QCheckBox::stateChanged, [&, name](int state) {
			name2trajectory[name].setVisible(state == Qt::Checked);
			if (state == Qt::Checked)
				name2trajectory[defaultTrajName].setVisible(false);
			else if (std::none_of(name2trajectory.begin(), name2trajectory.end(), [](auto& pair) { return pair.second.isVisible(); }))
				name2trajectory[defaultTrajName].setVisible(true);
		});
		ui->formLayout->addRow(box);
		assert(gradientTogglers.count(name) == 0);
		gradientTogglers[name] = box;
	};
	addCheckbox("gradient descent");
	addCheckbox("steepest descent");
	addCheckbox("conjugate gradient descent");
	addCheckbox("newton");
	addCheckbox("newton with minimization");

	auto toString = [](QuadraticFunction2d<double> const& func) {
		[[maybe_unused]] auto [center, vx, vy] = func.shift(func.c >= -0.1 ? -2 * func.c - 1 : 0).canonicalCoordSys();
		auto l1 = Len2(vx), l2 = Len2(vy), cond = std::max(l1, l2) / std::min(l1, l2);
		return QString::fromStdString((std::string)func + "\t(cond(A) = " + std::to_string(cond) + ")");
	};

	for (auto const& a : bifuncs)
		ui->functionComboBox->addItem(toString(a));

	isInit = true;
	recalc();
}

void MainWindow::multiMethodChanged(int) { recalc(); }
void MainWindow::methodChanged(int) { recalc(); }
void MainWindow::epsChanged(double) { recalc(); }
void MainWindow::powChanged(int) { recalc(); }
void MainWindow::toggleArrowheads(bool val) { for (auto &traj : name2trajectory) traj.second.toggleArrowheads(val); }
void MainWindow::toggleLevelSets(bool val) { for (auto &traj : name2trajectory) traj.second.toggleLevelSets(val); }
void MainWindow::startXChanged(double) { recalc(); }
void MainWindow::startYChanged(double) { recalc(); }
void MainWindow::functionChanged(int) { recalc(); }

void MainWindow::paintEvent(QPaintEvent* ev)
{
	QMainWindow::paintEvent(ev);

	QPainter painter(this);
	ev->accept();

	update();
}

MainWindow::~MainWindow()
{}
