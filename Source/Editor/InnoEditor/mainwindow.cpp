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
	ui->widgetInnoViewport->initialize();
	ui->comboBoxInnoRenderConfigurator->initialize();
	ui->widgetPropertyEditor->initialize();

	ui->treeWidgetInnoWorldExplorer->initialize(ui->widgetPropertyEditor);
    ui->directoryViewer->Initialize();
}
