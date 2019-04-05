#include "InnoViewport.h"
#include <qt_windows.h>

ICoreSystem* g_pCoreSystem;

InnoViewport::InnoViewport(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute( Qt::WA_TranslucentBackground);

    m_CoreSystem = new InnoCoreSystem();
    g_pCoreSystem = m_CoreSystem;
    m_GameInstance = new GameInstance();
    m_viewportEventFilter = new ViewportEventFilter();
}

InnoViewport::~InnoViewport()
{
    m_timerUpdate->stop();
    m_GameInstance->terminate();
    m_CoreSystem->terminate();
    delete m_GameInstance;
    delete m_CoreSystem;
}

void InnoViewport::initialize()
{
    installEventFilter(m_viewportEventFilter);

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

void InnoViewport::paintEvent(QPaintEvent *paintEvent)
{
    Update();
}

void InnoViewport::showEvent(QShowEvent *showEvent)
{
    QWidget::showEvent(showEvent);
}

void InnoViewport::resizeEvent(QResizeEvent *resizeEvent)
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

void InnoViewport::Update()
{
    m_GameInstance->update();
    m_CoreSystem->update();
}

void InnoViewport::Resize(float width, float height)
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

bool ViewportEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::KeyPress)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        g_pCoreSystem->getVisionSystem()->getWindowSystem()->sendEvent(WM_KEYDOWN, l_key->key(), 0);
    }
    if(event->type() == QEvent::KeyRelease)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        g_pCoreSystem->getVisionSystem()->getWindowSystem()->sendEvent(WM_KEYUP, l_key->key(), 0);
    }
    if(event->type() == QEvent::MouseButtonPress)
    {
        auto l_key = reinterpret_cast<QMouseEvent*>(event);
        switch (l_key->button()) {
        case Qt::MouseButton::LeftButton:
            g_pCoreSystem->getVisionSystem()->getWindowSystem()->sendEvent(WM_LBUTTONDOWN, WM_LBUTTONDOWN, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_pCoreSystem->getVisionSystem()->getWindowSystem()->sendEvent(WM_RBUTTONDOWN, WM_RBUTTONDOWN, 0);
            break;
        default:
            break;
        }
    }
    if(event->type() == QEvent::MouseButtonRelease)
    {
        auto l_mouseButton = reinterpret_cast<QMouseEvent*>(event);
        switch (l_mouseButton->button()) {
        case Qt::MouseButton::LeftButton:
            g_pCoreSystem->getVisionSystem()->getWindowSystem()->sendEvent(WM_LBUTTONUP, WM_LBUTTONUP, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_pCoreSystem->getVisionSystem()->getWindowSystem()->sendEvent(WM_RBUTTONUP, WM_RBUTTONUP, 0);
            break;
        default:
            break;
        }
    }
    if(event->type() == QEvent::MouseMove)
    {
        auto l_mouseMovement = reinterpret_cast<QMouseEvent*>(event);
        auto l_x = l_mouseMovement->localPos().x();
        auto l_y = l_mouseMovement->localPos().y();
        auto l_lparm = MAKELONG(l_x, l_y);
        g_pCoreSystem->getVisionSystem()->getWindowSystem()->sendEvent(WM_MOUSEMOVE, WM_MOUSEMOVE, l_lparm);
    }
    return false;
}
