#include "viewport.h"
#include <qt_windows.h>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include "../Engine/Platform/ApplicationEntry/ApplicationEntry.h"
#include "../Engine/Engine.h"

using namespace Inno;
Engine *g_Engine;
#define MOUSE_SENSITIVITY 16.0

Viewport::Viewport(QWidget *parent)
    : QWidget{ parent }
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    m_viewportEventFilter = new ViewportEventFilter();

    void* hInstance = reinterpret_cast<void*>(::GetModuleHandle(nullptr));
    WId l_hwnd = QWidget::winId();
    auto l_args = "-renderer 2 -mode 1 -loglevel 1";
    Inno::ApplicationEntry::Setup(hInstance, &l_hwnd, const_cast<char*>(l_args));
    Inno::ApplicationEntry::Initialize();

    auto l_engine = [&]() {
        Inno::ApplicationEntry::Run();
        Inno::ApplicationEntry::Terminate();
    };
    QFuture<void> future = QtConcurrent::run(l_engine);
}

Viewport::~Viewport()
{
    g_Engine->getWindowSystem()->SendEvent(WM_DESTROY, WM_DESTROY, 0);
}

void Viewport::initialize()
{
    installEventFilter(m_viewportEventFilter);
}

void Viewport::showEvent(QShowEvent *showEvent)
{
    QWidget::showEvent(showEvent);
}

void Viewport::resizeEvent(QResizeEvent *resizeEvent)
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

void Viewport::Resize(float width, float height)
{
    if (g_Engine)
    {
        if (g_Engine->GetStatus() == ObjectStatus::Activated)
        {
            TVec2<unsigned int> l_newResolution = TVec2<unsigned int>(width, height);
            g_Engine->Get<RenderingFrontend>()->SetScreenResolution(l_newResolution);
            g_Engine->getRenderingServer()->Resize();
        }
    }
}

bool ViewportEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    auto l_eventType = event->type();

    if (l_eventType == QEvent::KeyPress)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        if(l_key->key() < Qt::Key_Escape)
        {
            g_Engine->getWindowSystem()->SendEvent(WM_KEYDOWN, l_key->key(), 0);
        }

    }
    if (l_eventType == QEvent::KeyRelease)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        if(l_key->key() < Qt::Key_Escape)
        {
            g_Engine->getWindowSystem()->SendEvent(WM_KEYUP, l_key->key(), 0);
        }
    }
    if (l_eventType == QEvent::MouseButtonPress)
    {
        auto l_key = reinterpret_cast<QMouseEvent*>(event);
        switch (l_key->button())
        {
        case Qt::MouseButton::LeftButton:
            g_Engine->getWindowSystem()->SendEvent(WM_LBUTTONDOWN, WM_LBUTTONDOWN, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_Engine->getWindowSystem()->SendEvent(WM_RBUTTONDOWN, WM_RBUTTONDOWN, 0);
            break;
        default:
            break;
        }
    }
    if (l_eventType == QEvent::MouseButtonRelease)
    {
        auto l_mouseButton = reinterpret_cast<QMouseEvent*>(event);
        switch (l_mouseButton->button())
        {
        case Qt::MouseButton::LeftButton:
            g_Engine->getWindowSystem()->SendEvent(WM_LBUTTONUP, WM_LBUTTONUP, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_Engine->getWindowSystem()->SendEvent(WM_RBUTTONUP, WM_RBUTTONUP, 0);
            break;
        default:
            break;
        }
    }
    if (l_eventType == QEvent::MouseMove)
    {
        auto l_mouseMovement = reinterpret_cast<QMouseEvent*>(event);
        auto l_x = l_mouseMovement->pos().x() * MOUSE_SENSITIVITY;
        auto l_y = l_mouseMovement->pos().y() * MOUSE_SENSITIVITY;
        auto l_lparm = MAKELONG(l_x, l_y);
        g_Engine->getWindowSystem()->SendEvent(WM_MOUSEMOVE, WM_MOUSEMOVE, l_lparm);
    }
    return false;
}
