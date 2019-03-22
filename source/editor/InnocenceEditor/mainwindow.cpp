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

bool MainWindow::InitializeEngine()
{
    void* hInstance = (void*)::GetModuleHandle(NULL);
    ui->widgetViewport->Initialize(hInstance);
    return true;
}
