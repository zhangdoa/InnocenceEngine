#include "LinuxWindowSystem.h"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

#include<X11/X.h>
#include<X11/Xlib.h>
#include "glad/glad.h"
#include<GL/glx.h>

#undef Success

typedef GLXContext(*glXCreateContextAttribsARBProc) (Display*, GLXFBConfig, GLXContext, Bool, const int32_t*);

namespace LinuxWindowSystemNS
{
	bool Setup();
	bool Initialize();
	bool Update();
	bool Terminate();

	IWindowSurface* m_WindowSurface;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_InitConfig;
	std::vector<ButtonState> m_ButtonStates;
	std::set<WindowEventCallback*> m_WindowEventCallbacks;

	Display* m_display;
	Window m_window;
	GLint m_attributes[] = {
		GLX_RENDER_TYPE, GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
		GLX_DOUBLEBUFFER, true,
		GLX_RED_SIZE, 1,
		GLX_GREEN_SIZE, 1,
		GLX_BLUE_SIZE, 1,
		None
	};
	GLXContext m_context;
}

bool LinuxWindowSystemNS::Setup(ISystemConfig* systemConfig)
{
	m_display = XOpenDisplay(0);

	if (m_display == nullptr)
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "LinuxWindowSystem: Can't connect to X server!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	auto l_screenResolution = g_Engine->getRenderingFrontend()->GetScreenResolution();
	m_window = XCreateSimpleWindow(m_display, DefaultRootWindow(m_display),
		0, 0,   /* x, y */
		(uint32_t)l_screenResolution.x, (uint32_t)l_screenResolution.y, /* width, height */
		0, 0,     /* border_width, border */
		0);       /* background */

	if (!m_window)
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "LinuxWindowSystem: Can't create window!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	/* Show_the_window
	--------------- */
	auto l_windowName = g_Engine->GetApplicationName();
	XStoreName(m_display, m_window, l_windowName.c_str());
	XSelectInput(m_display, m_window, ExposureMask | StructureNotifyMask);

	XMapWindow(m_display, m_window);

	int32_t num_fbc = 0;
	GLXFBConfig* fbc = glXChooseFBConfig(m_display, DefaultScreen(m_display), m_attributes, &num_fbc);
	if (!fbc)
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "LinuxWindowSystem: glXChooseFBConfig() failed!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	/* Create old OpenGL context to get correct function pointer for
	glXCreateContextAttribsARB() */
	XVisualInfo* vi = glXGetVisualFromFBConfig(m_display, fbc[0]);
	GLXContext ctx_old = glXCreateContext(m_display, vi, 0, GL_TRUE);

	glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
	glXCreateContextAttribsARB =
		(glXCreateContextAttribsARBProc)
		glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");
	if (!glXCreateContextAttribsARB)
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "LinuxWindowSystem: glXCreateContextAttribsARB() not found!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	/* Destroy old context */
	glXMakeCurrent(m_display, 0, 0);
	glXDestroyContext(m_display, ctx_old);
	/* Set desired minimum OpenGL version */
	static int32_t context_attribs[] = {
		GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
		GLX_CONTEXT_MINOR_VERSION_ARB, 5,
		None
	};

	/* Create modern OpenGL context */
	m_context = glXCreateContextAttribsARB(m_display, fbc[0], NULL, true, context_attribs);
	if (!m_context)
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "LinuxWindowSystem: Failed to create OpenGL context!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
	glXMakeCurrent(m_display, m_window, m_context);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGL())
	{
		g_Engine->getLogSystem()->Log(LogLevel::Error, "LinuxWindowSystem: Failed to Initialize GLAD.");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	LinuxWindowSystemNS::m_ObjectStatus = ObjectStatus::Activated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "LinuxWindowSystem Setup finished.");

	return true;
}

bool LinuxWindowSystemNS::Initialize()
{
	g_Engine->getLogSystem()->Log(LogLevel::Success, "LinuxWindowSystem has been initialized.");
	return true;
}

bool LinuxWindowSystemNS::Update()
{
	return true;
}

bool LinuxWindowSystemNS::Terminate()
{
	LinuxWindowSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "LinuxWindowSystem has been terminated.");
	return true;
}

bool LinuxWindowSystem::Setup(void* hInstance, void* hwnd)
{
	return LinuxWindowSystemNS::Setup(ISystemConfig * systemConfig);
}

bool LinuxWindowSystem::Initialize()
{
	return LinuxWindowSystemNS::Initialize();
}

bool LinuxWindowSystem::Update()
{
	return LinuxWindowSystemNS::Update();
}

bool LinuxWindowSystem::Terminate()
{
	return LinuxWindowSystemNS::Terminate();
}

ObjectStatus LinuxWindowSystem::GetStatus()
{
	return LinuxWindowSystemNS::m_ObjectStatus;
}

IWindowSurface* LinuxWindowSystem::GetWindowSurface()
{
	return LinuxWindowSystemNS::m_WindowSurface;
}

const std::vector<ButtonState>& LinuxWindowSystem::GetButtonState()
{
	return LinuxWindowSystemNS::m_ButtonStates;
}

bool LinuxWindowSystem::SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
	return true;
}

bool LinuxWindowSystem::AddEventCallback(WindowEventCallback* callback)
{
	LinuxWindowSystemNS::m_WindowEventCallbacks.emplace(functor);
	return true;
}

//void LinuxWindowSystem::swapBuffer()
//{
	//glXSwapBuffers(LinuxWindowSystemNS::m_display, LinuxWindowSystemNS::m_window);
//}