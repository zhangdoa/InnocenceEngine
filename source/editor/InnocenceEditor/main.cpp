#include "mainwindow.h"
#include <QApplication>
#include "DarkStyle.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(new DarkStyle);

    MainWindow w;
    w.show();

    return a.exec();
}
