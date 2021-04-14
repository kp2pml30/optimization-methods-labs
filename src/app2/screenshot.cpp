#include "ui_mainwindow.h"
#include "./mainwindow.h"

#include <QImage>
#include <QPainter>

void MainWindow::screenshot()
{
	auto image = QImage(ui->visualChartView->width(), ui->visualChartView->height(), QImage::Format::Format_RGB888);
	image.fill(0xffffff);
	auto painter = QPainter(&image);
	ui->visualChartView->render(&painter);
	image.save("screenshot.png");
}
