#include "InnoWindowSurface.h"
#include <qt_windows.h>

InnoWindowSurface::InnoWindowSurface(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);

    m_CoreSystem = new InnoCoreSystem();
    m_GameInstance = new GameInstance();
}

InnoWindowSurface::~InnoWindowSurface()
{
    m_timerUpdate->stop();
    m_GameInstance->terminate();
    m_CoreSystem->terminate();
    delete m_GameInstance;
    delete m_CoreSystem;
}

void InnoWindowSurface::initializeEngine()
{
    m_timerUpdate = new QTimer(this);
    connect(m_timerUpdate, SIGNAL(timeout()), this, SLOT(Update()));

    WId l_hwnd = QWidget::winId();
    void* hInstance = (void*)::GetModuleHandle(NULL);
    const char* l_args = "-renderer 0 -mode 1";

    m_CoreSystem->setup(hInstance, &l_hwnd, (char*)l_args);
    m_GameInstance->setup();

    m_CoreSystem->initialize();
    m_GameInstance->initialize();

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
    if (resizeEvent->oldSize() == resizeEvent->size())
        return;

    int width = this->size().width();
    int height = this->size().height();

    if (width % 2 != 0)
        width++;

    if (height % 2 != 0)
        height++;

    // Change the size of the widget
    this->setGeometry(QRect(0, 0, width, height));

    Resize(width, height);
}

void InnoWindowSurface::Update()
{
    m_GameInstance->update();
    m_CoreSystem->update();
}

void InnoWindowSurface::Resize(float width, float height)
{
    if(m_CoreSystem)
    {
        if(m_CoreSystem->getStatus() == ObjectStatus::ALIVE)
        {
            TVec2<unsigned int> l_newResolution = TVec2<unsigned int>(width, height);
            m_CoreSystem->getVisionSystem()->getRenderingFrontend()->setScreenResolution(l_newResolution);
            m_CoreSystem->getVisionSystem()->getRenderingBackend()->resize();
        }
    }
}
