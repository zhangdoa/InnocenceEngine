#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <qt_windows.h>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	
	// Set window to maximize on startup and center it properly
	this->setWindowState(Qt::WindowMaximized);
	
	// Alternative: if you prefer setting to available screen geometry
	// QRect screenGeometry = QApplication::desktop()->availableGeometry();
	// this->setGeometry(screenGeometry);
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
