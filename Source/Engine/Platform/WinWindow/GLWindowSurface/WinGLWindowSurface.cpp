#include "WinGLWindowSurface.h"
#include "../WinWindowSystem.h"
#include "../../../Engine/Core/InnoLogger.h"

#include "glad/gl.h"
#include "glext.h"
#include "wglext.h"

#include "../../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

namespace WinGLWindowSurfaceNS
{
	bool Setup(ISystemConfig* systemConfig);
	bool Initialize();
	bool Update();
	bool Terminate();

	HDC m_HDC;
	HGLRC m_HGLRC;
	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
	InitConfig m_initConfig;
}

bool WinGLWindowSurfaceNS::Setup(ISystemConfig* systemConfig)
{
	auto l_windowSurfaceConfig = reinterpret_cast<IWindowSurfaceConfig*>(systemConfig);

	m_initConfig = g_Engine->getInitConfig();

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = (WNDPROC)l_windowSurfaceConfig->WindowProc;
	wcex.hInstance = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHInstance();
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getApplicationName();

	auto l_windowClass = MAKEINTATOM(RegisterClassEx(&wcex));

	// create temporary window
	HWND fakeWND = CreateWindow(
		l_windowClass,
		"Fake Window",
		WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0, 0,						// position x, y
		1, 1,						// width, height
		NULL, NULL,					// parent window, menu
		reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHInstance(), NULL);			// instance, param

	HDC fakeDC = GetDC(fakeWND);	// Device Context

	PIXELFORMATDESCRIPTOR fakePFD;
	ZeroMemory(&fakePFD, sizeof(fakePFD));
	fakePFD.nSize = sizeof(fakePFD);
	fakePFD.nVersion = 1;
	fakePFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	fakePFD.iPixelType = PFD_TYPE_RGBA;
	fakePFD.cColorBits = 32;
	fakePFD.cAlphaBits = 8;
	fakePFD.cDepthBits = 24;

	const int32_t fakePFDID = ChoosePixelFormat(fakeDC, &fakePFD);

	if (fakePFDID == 0)
	{
		m_ObjectStatus = ObjectStatus::Created;
		InnoLogger::Log(LogLevel::Error, "WinWindowSystem: ChoosePixelFormat() failed.");
		return false;
	}

	if (SetPixelFormat(fakeDC, fakePFDID, &fakePFD) == false)
	{
		m_ObjectStatus = ObjectStatus::Created;
		InnoLogger::Log(LogLevel::Error, "WinWindowSystem: SetPixelFormat() failed.");
		return false;
	}

	HGLRC fakeRC = wglCreateContext(fakeDC);

	// Rendering Context
	if (fakeRC == 0)
	{
		m_ObjectStatus = ObjectStatus::Created;
		InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglCreateContext() failed.");
		return false;
	}

	if (wglMakeCurrent(fakeDC, fakeRC) == false)
	{
		m_ObjectStatus = ObjectStatus::Created;
		InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglMakeCurrent() failed.");
		return false;
	}

	// load function pointer for context creation
	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = nullptr;
	wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(wglGetProcAddress("wglChoosePixelFormatARB"));
	if (wglChoosePixelFormatARB == nullptr)
	{
		m_ObjectStatus = ObjectStatus::Created;
		InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglGetProcAddress(wglChoosePixelFormatARB) failed.");
		return false;
	}

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
	wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(wglGetProcAddress("wglCreateContextAttribsARB"));
	if (wglCreateContextAttribsARB == nullptr)
	{
		m_ObjectStatus = ObjectStatus::Created;
		InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglGetProcAddress(wglCreateContextAttribsARB) failed.");
		return false;
	}

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		// Determine the resolution of the clients desktop screen.
		auto l_screenResolution = g_Engine->getRenderingFrontend()->getScreenResolution();
		auto l_screenWidth = (int32_t)l_screenResolution.x;
		auto l_screenHeight = (int32_t)l_screenResolution.y;

		RECT l_rect;
		l_rect.right = (GetSystemMetrics(SM_CXSCREEN) - l_screenWidth) / 2;
		l_rect.bottom = (GetSystemMetrics(SM_CYSCREEN) - l_screenHeight) / 2;

		AdjustWindowRect(&l_rect, WS_OVERLAPPEDWINDOW, false);

		// create a new window and context
		auto l_hwnd = CreateWindow(
			l_windowClass, reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getApplicationName(), // class name, window name
			WS_OVERLAPPEDWINDOW, // styles
			l_rect.right, l_rect.bottom, // posx, posy. If x is set to CW_USEDEFAULT y is ignored
			l_screenWidth, l_screenHeight, // width, height
			NULL, NULL, // parent window, menu
			reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHInstance(), NULL); // instance, param

		reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->setHwnd(l_hwnd);
	}

	auto f_CreateGLContextTask = [&]()
	{
		m_HDC = GetDC(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd());

		const int32_t pixelAttribs[] = {
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_ALPHA_BITS_ARB, 8,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
			WGL_SAMPLES_ARB, 4,
			0
		};

		int32_t pixelFormatID;
		UINT numFormats;
		const bool status = wglChoosePixelFormatARB(m_HDC, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);

		if (status == false || numFormats == 0)
		{
			m_ObjectStatus = ObjectStatus::Created;
			InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglChoosePixelFormatARB() failed.");
		}

		PIXELFORMATDESCRIPTOR PFD;
		DescribePixelFormat(m_HDC, pixelFormatID, sizeof(PFD), &PFD);
		SetPixelFormat(m_HDC, pixelFormatID, &PFD);

		const int32_t major_min = 4, minor_min = 6;
		std::vector<int32_t> contextAttribs =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, major_min,
			WGL_CONTEXT_MINOR_VERSION_ARB, minor_min,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			WGL_CONTEXT_FLAGS_ARB
		};

#ifdef INNO_DEBUG
		contextAttribs.emplace_back(WGL_CONTEXT_DEBUG_BIT_ARB);
#endif // INNO_DEBUG
		contextAttribs.emplace_back(0);

		m_HGLRC = wglCreateContextAttribsARB(m_HDC, 0, &contextAttribs[0]);

		if (m_HGLRC == NULL)
		{
			m_ObjectStatus = ObjectStatus::Created;
			InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglCreateContextAttribsARB() failed.");
		}
	};

	auto l_CreateGLContextTask = g_Engine->getTaskSystem()->Submit("CreateGLContextTask", 2, nullptr, f_CreateGLContextTask);
	l_CreateGLContextTask.m_Future->Get();

	// delete temporary context and window
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeRC);
	ReleaseDC(fakeWND, fakeDC);
	DestroyWindow(fakeWND);

	auto f_ActivateGLContextTask = [&]()
	{
		if (!wglMakeCurrent(m_HDC, m_HGLRC))
		{
			m_ObjectStatus = ObjectStatus::Created;
			InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglMakeCurrent() failed.");
		}

		// init opengl loader here (extra safe version)
		// glad: load all OpenGL function pointers
		// ---------------------------------------
		if (!gladLoadGL(nullptr))
		{
			m_ObjectStatus = ObjectStatus::Created;
			InnoLogger::Log(LogLevel::Error, "WinWindowSystem: Failed to Initialize GLAD.");
		}

		auto l_renderingConfig = g_Engine->getRenderingFrontend()->getRenderingConfig();

		if (!l_renderingConfig.VSync)
		{
			PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = nullptr;
			wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(wglGetProcAddress("wglSwapIntervalEXT"));
			if (wglSwapIntervalEXT == nullptr)
			{
				m_ObjectStatus = ObjectStatus::Created;
				InnoLogger::Log(LogLevel::Error, "WinWindowSystem: wglGetProcAddress(wglSwapIntervalEXT) failed.");
			}
			wglSwapIntervalEXT(0);
		}
	};

	auto l_ActivateGLContextTask = g_Engine->getTaskSystem()->Submit("ActivateGLContextTask", 2, nullptr, f_ActivateGLContextTask);
	l_ActivateGLContextTask.m_Future->Get();

	if (m_initConfig.engineMode == EngineMode::Host)
	{
		ShowWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd(), true);
		SetForegroundWindow(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd());
		SetFocus(reinterpret_cast<WinWindowSystem*>(g_Engine->getWindowSystem())->getHwnd());
	}

	m_ObjectStatus = ObjectStatus::Activated;
	InnoLogger::Log(LogLevel::Success, "WinGLWindowSurface Setup finished.");

	return true;
}

bool WinGLWindowSurfaceNS::Initialize()
{
	InnoLogger::Log(LogLevel::Success, "WinGLWindowSurface has been initialized.");
	return true;
}

bool WinGLWindowSurfaceNS::Update()
{
	return true;
}

bool WinGLWindowSurfaceNS::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "WinGLWindowSurfaceNS has been terminated.");

	return true;
}

bool WinGLWindowSurface::Setup(ISystemConfig* systemConfig)
{
	return WinGLWindowSurfaceNS::Setup(systemConfig);
}

bool WinGLWindowSurface::Initialize()
{
	return WinGLWindowSurfaceNS::Initialize();
}

bool WinGLWindowSurface::Update()
{
	return WinGLWindowSurfaceNS::Update();
}

bool WinGLWindowSurface::Terminate()
{
	return WinGLWindowSurfaceNS::Terminate();
}

ObjectStatus WinGLWindowSurface::GetStatus()
{
	return WinGLWindowSurfaceNS::m_ObjectStatus;
}

bool WinGLWindowSurface::swapBuffer()
{
	SwapBuffers(WinGLWindowSurfaceNS::m_HDC);

	return true;
}