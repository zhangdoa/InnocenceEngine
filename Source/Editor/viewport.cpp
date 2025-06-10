#include "viewport.h"
#include <qt_windows.h>
#include <QFuture>
#include <QtConcurrent/QtConcurrentRun>
#include "../Engine/Interface/IWindowSystem.h"
#include "../Engine/RenderingServer/IRenderingServer.h"
#include "../Engine/Services/RenderingConfigurationService.h"

using namespace Inno;

#define MOUSE_SENSITIVITY 16.0

Viewport::Viewport(QWidget* parent)
    : QWidget{ parent }
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);

    m_HWND = QWidget::winId();
    m_viewportEventFilter = new ViewportEventFilter(m_HWND);

    void* hInstance = reinterpret_cast<void*>(::GetModuleHandle(nullptr));
    auto l_args = "-renderer 0 -mode 1 -loglevel 1";

    m_pEngine = std::make_unique<Engine>();

    m_pEngine->Setup(hInstance, &m_HWND, const_cast<char*>(l_args));
    m_pEngine->Initialize();

    auto l_engine = [&]() {
        m_pEngine->Run();
        m_pEngine->Terminate();
        };

    QFuture<void> future = QtConcurrent::run(l_engine);
}

Viewport::~Viewport()
{
    g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_DESTROY, WM_DESTROY, 0);
}

void Viewport::initialize()
{
    installEventFilter(m_viewportEventFilter);
}

void Viewport::showEvent(QShowEvent* showEvent)
{
    QWidget::showEvent(showEvent);
}

void Viewport::resizeEvent(QResizeEvent* resizeEvent)
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
    if (!g_Engine)
        return;

    if (g_Engine->GetStatus() != ObjectStatus::Activated)
        return;

    if (width == 0.0 || height == 0.0)
        return;
    
    TVec2<unsigned int> l_newResolution = TVec2<unsigned int>(width, height);
    g_Engine->Get<RenderingConfigurationService>()->SetScreenResolution(l_newResolution);
    g_Engine->getRenderingServer()->Resize();
}

bool ViewportEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    auto l_eventType = event->type();
    if (l_eventType == QEvent::KeyPress)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        if (l_key->key() < Qt::Key_Escape)
        {
            g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_KEYDOWN, l_key->key(), 0);
        }

    }
    if (l_eventType == QEvent::KeyRelease)
    {
        auto l_key = reinterpret_cast<QKeyEvent*>(event);
        if (l_key->key() < Qt::Key_Escape)
        {
            g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_KEYUP, l_key->key(), 0);
        }
    }
    if (l_eventType == QEvent::MouseButtonPress)
    {
        auto l_key = reinterpret_cast<QMouseEvent*>(event);
        switch (l_key->button())
        {
        case Qt::MouseButton::LeftButton:
            g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_LBUTTONDOWN, WM_LBUTTONDOWN, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_RBUTTONDOWN, WM_RBUTTONDOWN, 0);
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
            g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_LBUTTONUP, WM_LBUTTONUP, 0);
            break;
        case Qt::MouseButton::RightButton:
            g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_RBUTTONUP, WM_RBUTTONUP, 0);
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
        g_Engine->getWindowSystem()->SendEvent(reinterpret_cast<void*>(m_HWND), WM_MOUSEMOVE, WM_MOUSEMOVE, l_lparm);
    }
    return false;
}
