#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qt_windows.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeEngine()
{
    ui->widgetViewport->initializeEngine();
}
