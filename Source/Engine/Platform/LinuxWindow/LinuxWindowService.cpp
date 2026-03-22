#include "LinuxWindowService.h"

#include "../../Engine.h"

using namespace Inno;
;

#include<X11/X.h>
#include<X11/Xlib.h>
#include "glad/glad.h"
#include<GL/glx.h>

#undef Success

typedef GLXContext(*glXCreateContextAttribsARBProc) (Display*, GLXFBConfig, GLXContext, Bool, const int32_t*);

namespace LinuxWindowServiceNS
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

bool LinuxWindowServiceNS::Setup(IServiceConfig* systemConfig)
{
	m_display = XOpenDisplay(0);

	if (m_display == nullptr)
	{
		Log(Error, "Can't connect to X server!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	m_window = XCreateSimpleWindow(m_display, DefaultRootWindow(m_display),
		0, 0,   /* x, y */
		(uint32_t)l_screenResolution.x, (uint32_t)l_screenResolution.y, /* width, height */
		0, 0,     /* border_width, border */
		0);       /* background */

	if (!m_window)
	{
		Log(Error, "Can't create window!");
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
		Log(Error, "glXChooseFBConfig() failed!");
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
		Log(Error, "glXCreateContextAttribsARB() not found!");
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
		Log(Error, "Failed to create OpenGL context!");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
	glXMakeCurrent(m_display, m_window, m_context);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGL())
	{
		Log(Error, "Failed to Initialize GLAD.");
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	LinuxWindowServiceNS::m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "LinuxWindowService Setup finished.");

	return true;
}

bool LinuxWindowServiceNS::Initialize()
{
	Log(Success, "LinuxWindowService has been initialized.");
	return true;
}

bool LinuxWindowServiceNS::Update()
{
	return true;
}

bool LinuxWindowServiceNS::Terminate()
{
	LinuxWindowServiceNS::m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "LinuxWindowService has been terminated.");
	return true;
}

bool LinuxWindowService::Setup(void* hInstance, void* hwnd)
{
	return LinuxWindowServiceNS::Setup(IServiceConfig * systemConfig);
}

bool LinuxWindowService::Initialize()
{
	return LinuxWindowServiceNS::Initialize();
}

bool LinuxWindowService::Update()
{
	return LinuxWindowServiceNS::Update();
}

bool LinuxWindowService::Terminate()
{
	return LinuxWindowServiceNS::Terminate();
}

ObjectStatus LinuxWindowService::GetStatus()
{
	return LinuxWindowServiceNS::m_ObjectStatus;
}

IWindowSurface* LinuxWindowService::GetWindowSurface()
{
	return LinuxWindowServiceNS::m_WindowSurface;
}

const std::vector<ButtonState>& LinuxWindowService::GetButtonState()
{
	return LinuxWindowServiceNS::m_ButtonStates;
}

bool LinuxWindowService::SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam)
{
	return true;
}

bool LinuxWindowService::AddEventCallback(WindowEventCallback* callback)
{
	LinuxWindowServiceNS::m_WindowEventCallbacks.emplace(functor);
	return true;
}

//void LinuxWindowService::swapBuffer()
//{
	//glXSwapBuffers(LinuxWindowServiceNS::m_display, LinuxWindowServiceNS::m_window);
//}