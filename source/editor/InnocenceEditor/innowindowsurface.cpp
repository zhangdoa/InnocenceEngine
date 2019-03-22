#include "InnoWindowSurface.h"

InnoWindowSurface::InnoWindowSurface(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_timerUpdate = new QTimer(this);
    connect(m_timerUpdate, SIGNAL(timeout()), this, SLOT(update()));
}

InnoWindowSurface::~InnoWindowSurface()
{
    m_timerUpdate->stop();
    //InnoApplication::terminate();
}

void InnoWindowSurface::Initialize(void *hinstance)
{
    m_hinstance = hinstance;
}

void InnoWindowSurface::terminate()
{

}

void InnoWindowSurface::initializeGL()
{
    initializeOpenGLFunctions();
    const char* l_args = "-renderer 0 -mode 1";
    glEnable(Multisample);
    //InnoApplication::setup(m_hinstance, nullptr, (char *)l_args, 0);
    //InnoApplication::initialize();
    m_timerUpdate->start(0);
}

void InnoWindowSurface::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    //InnoApplication::update();
}

void InnoWindowSurface::resizeGL(int w, int h)
{

}
