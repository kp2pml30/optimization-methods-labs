#include "ui_mainwindow.h"
#include "mainwindow.h"

#include <QMouseEvent>
#include <QTimer>

#include <iostream>
#include <qnamespace.h>

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this); // this line gives dataraces even in empty solution >_<
}

void MainWindow::paintEvent(QPaintEvent* ev)
{
	QMainWindow::paintEvent(ev);

	QPainter painter(this);
	ev->accept();
}

MainWindow::~MainWindow()
{}
