#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "qt_wsi_platform.h"

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow(parent) {
    ui = new Ui::MainWindow();
    ui->setupUi(this);
}

void MainWindow::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    vkPane = new VulkanPane();
    vkPane->setSurfaceType(QSurface::VulkanSurface);
    setCentralWidget(QWidget::createWindowContainer(vkPane, this));
}

void MainWindow::resetTriggered() {
    vkPane->getEmulatorThread().reset();
}
