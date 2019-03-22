#include "mainwindow.h"
#include <QApplication>
#include "DarkStyle.h"
#include <QSurfaceFormat>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    //a.setStyle(new DarkStyle);

    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
        fmt.setVersion(4, 5);
        fmt.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(fmt);

    MainWindow w;
    w.InitializeEngine();
    w.show();

    return a.exec();
}
