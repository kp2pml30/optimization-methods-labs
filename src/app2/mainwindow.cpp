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

	ui->function->setText(QString::fromStdString(static_cast<std::string>(bifunc)));

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

void MainWindow::paintEvent(QPaintEvent* ev)
{
	QMainWindow::paintEvent(ev);

	QPainter painter(this);
	ev->accept();

	update();
}

MainWindow::~MainWindow()
{}
