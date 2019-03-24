#include "InnoWindowSurface.h"
#include <qt_windows.h>

InnoWindowSurface::InnoWindowSurface(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    //setAttribute(Qt::WA_NoSystemBackground);
}

InnoWindowSurface::~InnoWindowSurface()
{
    m_timerUpdate->stop();
    //m_GameInstance->terminate();
    //m_CoreSystem->terminate();
    //delete m_GameInstance;
    //delete m_CoreSystem;
}

void InnoWindowSurface::initializeEngine()
{
    m_timerUpdate = new QTimer(this);
    connect(m_timerUpdate, SIGNAL(timeout()), this, SLOT(Update()));

    WId l_hwnd = QWidget::winId();
    void* hInstance = (void*)::GetModuleHandle(NULL);
    const char* l_args = "-renderer 0 -mode 1";

    //m_CoreSystem = new InnoCoreSystem();
    //m_GameInstance = new GameInstance();

    //m_CoreSystem->setup(hInstance, &l_hwnd, (char*)l_args);
    //m_GameInstance->setup();

    //m_CoreSystem->initialize();
    //m_GameInstance->initialize();

    m_timerUpdate->start(16);
}

void InnoWindowSurface::paintEvent(QPaintEvent *paintEvent)
{
    Update();
}

void InnoWindowSurface::showEvent(QShowEvent *showEvent)
{
    QWidget::showEvent(showEvent);
}

void InnoWindowSurface::resizeEvent(QResizeEvent *resizeEvent)
{
    QWidget::resizeEvent(resizeEvent);
    auto sz = resizeEvent->size();
    if((sz.width() < 0) || (sz.height() < 0))
        return;
    m_CoreSystem->getVisionSystem()->getRenderingBackend()->resize();
}

void InnoWindowSurface::Update()
{
    //m_GameInstance->update();
    //m_CoreSystem->update();
}

