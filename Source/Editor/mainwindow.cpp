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
	ui->widgetViewport->initialize();
	ui->comboBoxRenderConfigurator->initialize();
	ui->widgetPropertyEditor->initialize();

	ui->treeWidgetWorldExplorer->initialize(ui->widgetPropertyEditor);
    ui->directoryViewer->Initialize();
}
