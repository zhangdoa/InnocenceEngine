#include "innoviewport.h"
#include <qt_windows.h>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include "../../Engine/Common/InnoApplication.h"
#include "../../Engine/System/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

InnoViewport::InnoViewport(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute( Qt::WA_TranslucentBackground);

    m_viewportEventFilter = new ViewportEventFilter();

    void* hInstance = (void*)::GetModuleHandle(NULL);
    WId l_hwnd = QWidget::winId();
    const char* l_args = "-renderer 1 -mode 1";
    InnoApplication::Setup(hInstance, &l_hwnd, (char*)l_args);
    InnoApplication::Initialize();

    auto l_engine = [&](){
        InnoApplication::Run();
        InnoApplication::Terminate();
    };
    QFuture<void> future = QtConcurrent::run(l_engine);
}

InnoViewport::~InnoViewport()
{
    g_pCoreSystem->getWindowSystem()->sendEvent(WM_DESTROY, WM_DESTROY, 0);
}

void InnoViewport::initialize()
{
    installEventFilter(m_viewportEventFilter);
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

void InnoViewport::Resize(float width, float height)
{
    if(g_pCoreSystem)
    {
        if(g_pCoreSystem->getStatus() == ObjectStatus::Activated)
        {
            TVec2<unsigned int> l_newResolution = TVec2<unsigned int>(width, height);
            g_pCoreSystem->getRenderingFrontend()->setScreenResolution(l_newResolution);
            g_pCoreSystem->getRenderingBackend()->resize();
        }
    }
}

bool ViewportEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    auto l_eventType = event->type();
    if(l_eventType == QEvent::KeyPress)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        g_pCoreSystem->getWindowSystem()->sendEvent(WM_KEYDOWN, l_key->key(), 0);
    }
    if(l_eventType == QEvent::KeyRelease)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        g_pCoreSystem->getWindowSystem()->sendEvent(WM_KEYUP, l_key->key(), 0);
    }
    if(l_eventType == QEvent::MouseButtonPress)
    {
        auto l_key = reinterpret_cast<QMouseEvent*>(event);
        switch (l_key->button()) {
        case Qt::MouseButton::LeftButton:
            g_pCoreSystem->getWindowSystem()->sendEvent(WM_LBUTTONDOWN, WM_LBUTTONDOWN, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_pCoreSystem->getWindowSystem()->sendEvent(WM_RBUTTONDOWN, WM_RBUTTONDOWN, 0);
            break;
        default:
            break;
        }
    }
    if(l_eventType == QEvent::MouseButtonRelease)
    {
        auto l_mouseButton = reinterpret_cast<QMouseEvent*>(event);
        switch (l_mouseButton->button()) {
        case Qt::MouseButton::LeftButton:
            g_pCoreSystem->getWindowSystem()->sendEvent(WM_LBUTTONUP, WM_LBUTTONUP, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_pCoreSystem->getWindowSystem()->sendEvent(WM_RBUTTONUP, WM_RBUTTONUP, 0);
            break;
        default:
            break;
        }
    }
    if(l_eventType == QEvent::MouseMove)
    {
        auto l_mouseMovement = reinterpret_cast<QMouseEvent*>(event);
        auto l_x = l_mouseMovement->localPos().x();
        auto l_y = l_mouseMovement->localPos().y();
        auto l_lparm = MAKELONG(l_x, l_y);
        g_pCoreSystem->getWindowSystem()->sendEvent(WM_MOUSEMOVE, WM_MOUSEMOVE, l_lparm);
    }
    return false;
}
