#ifndef INNOWINDOWSURFACE_H
#define INNOWINDOWSURFACE_H

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "../../engine/common/InnoApplication.h"

class InnoWindowSurface: public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    InnoWindowSurface(QWidget *parent = 0);
    ~InnoWindowSurface();
    void Initialize(void *hinstance);
    void terminate();
protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void* m_hinstance;
    bool m_animating;
    QTimer* m_timerUpdate;

signals:
};

#endif // INNOWINDOWSURFACE_H
