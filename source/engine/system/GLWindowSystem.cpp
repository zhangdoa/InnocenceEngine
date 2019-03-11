#include "GLWindowSystem.h"

#include "../component/WindowSystemComponent.h"
#include "../component/GLWindowSystemComponent.h"
#include "GLFWWrapper.h"

#include "InputSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE GLWindowSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	static WindowSystemComponent* g_WindowSystemComponent;
	static GLWindowSystemComponent* g_GLWindowSystemComponent;

	void hideMouseCursor();
	void showMouseCursor();
}

INNO_SYSTEM_EXPORT bool GLWindowSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	GLWindowSystemNS::g_WindowSystemComponent = &WindowSystemComponent::get();
	GLWindowSystemNS::g_GLWindowSystemComponent = &GLWindowSystemComponent::get();

	GLWindowSystemNS::g_WindowSystemComponent->m_windowName = g_pCoreSystem->getGameSystem()->getGameName();

	//setup window
	if (glfwInit() != GL_TRUE)
	{
		GLWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLWindowSystem: Failed to initialize GLFW.");
		return false;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#ifdef INNO_PLATFORM_MAC
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

	// Open a window and create its OpenGL context
	GLWindowSystemNS::g_GLWindowSystemComponent->m_window = glfwCreateWindow((int)GLWindowSystemNS::g_WindowSystemComponent->m_windowResolution.x, (int)GLWindowSystemNS::g_WindowSystemComponent->m_windowResolution.y, GLWindowSystemNS::g_WindowSystemComponent->m_windowName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(GLWindowSystemNS::g_GLWindowSystemComponent->m_window);
	if (GLWindowSystemNS::g_GLWindowSystemComponent->m_window == nullptr) {
		GLWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLWindowSystem: Failed to open GLFW window.");
		glfwTerminate();
		return false;
	}
	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		GLWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "GLWindowSystem: Failed to initialize GLAD.");
		return false;
	}

	//setup input
	glfwSetInputMode(GLWindowSystemNS::g_GLWindowSystemComponent->m_window, GLFW_STICKY_KEYS, GL_TRUE);

	GLWindowSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool GLWindowSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::get().initialize(GLWindowSystemNS::g_GLWindowSystemComponent->m_window, g_pCoreSystem->getInputSystem());

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLWindowSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool GLWindowSystem::update()
{
	//update window
	if (GLWindowSystemNS::g_GLWindowSystemComponent->m_window == nullptr || glfwWindowShouldClose(GLWindowSystemNS::g_GLWindowSystemComponent->m_window) != 0)
	{
		GLWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "GLWindowSystem: Input error or Window closed.");
	}
	else
	{
		glfwPollEvents();

		windowCallbackWrapper::get().update(GLWindowSystemNS::g_GLWindowSystemComponent->m_window);
	}

	return true;
}

INNO_SYSTEM_EXPORT bool GLWindowSystem::terminate()
{
	glfwSetInputMode(GLWindowSystemNS::g_GLWindowSystemComponent->m_window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwDestroyWindow(GLWindowSystemNS::g_GLWindowSystemComponent->m_window);
	glfwTerminate();

	GLWindowSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GLWindowSystem has been terminated.");
	return true;
}

void GLWindowSystem::swapBuffer()
{
	glfwSwapBuffers(GLWindowSystemNS::g_GLWindowSystemComponent->m_window);
}

void GLWindowSystemNS::hideMouseCursor()
{
	glfwSetInputMode(GLWindowSystemNS::g_GLWindowSystemComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void GLWindowSystemNS::showMouseCursor()
{
	glfwSetInputMode(GLWindowSystemNS::g_GLWindowSystemComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

INNO_SYSTEM_EXPORT ObjectStatus GLWindowSystem::getStatus()
{
	return GLWindowSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT ButtonStatusMap GLWindowSystem::getButtonStatus()
{
	return windowCallbackWrapper::get().getButtonStatus();
}
