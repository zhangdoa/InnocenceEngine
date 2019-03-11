#include "VKWindowSystem.h"

#include "../component/WindowSystemComponent.h"
#include "../component/VKWindowSystemComponent.h"
#include "GLFWWrapper.h"

#include "InputSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKWindowSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	static WindowSystemComponent* g_WindowSystemComponent;
	static VKWindowSystemComponent* g_VKWindowSystemComponent;

	ButtonStatusMap m_buttonStatus;

	bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);

	void hideMouseCursor();
	void showMouseCursor();
}

bool VKWindowSystemNS::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	g_WindowSystemComponent = &WindowSystemComponent::get();
	g_VKWindowSystemComponent = &VKWindowSystemComponent::get();

	g_WindowSystemComponent->m_windowName = g_pCoreSystem->getGameSystem()->getGameName();

	//setup window
	if (glfwInit() != GL_TRUE)
	{
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to initialize GLFW.");
		return false;
	}

	// bind Vulkan API hook later
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Open a window
	g_VKWindowSystemComponent->m_window = glfwCreateWindow((int)g_WindowSystemComponent->m_windowResolution.x, (int)g_WindowSystemComponent->m_windowResolution.y, g_WindowSystemComponent->m_windowName.c_str(), NULL, NULL);

	glfwMakeContextCurrent(g_VKWindowSystemComponent->m_window);

	if (g_VKWindowSystemComponent->m_window == nullptr) {
		m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to open GLFW window.");
		glfwTerminate();
		return false;
	}

	//setup input
	glfwSetInputMode(g_VKWindowSystemComponent->m_window, GLFW_STICKY_KEYS, GL_TRUE);

	m_objectStatus = ObjectStatus::ALIVE;

	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	return VKWindowSystemNS::setup(hInstance, hPrevInstance, pScmdline, nCmdshow);
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::get().initialize(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, g_pCoreSystem->getInputSystem());

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKWindowSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::update()
{
	//update window
	if (VKWindowSystemNS::g_VKWindowSystemComponent->m_window == nullptr || glfwWindowShouldClose(VKWindowSystemNS::g_VKWindowSystemComponent->m_window) != 0)
	{
		VKWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VKWindowSystem: Input error or Window closed.");
	}
	else
	{
		glfwPollEvents();

		windowCallbackWrapper::get().update(VKWindowSystemNS::g_VKWindowSystemComponent->m_window);
	}

	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::terminate()
{
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwDestroyWindow(VKWindowSystemNS::g_VKWindowSystemComponent->m_window);
	glfwTerminate();

	VKWindowSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKWindowSystem has been terminated.");
	return true;
}

void VKWindowSystem::swapBuffer()
{
	glfwSwapBuffers(VKWindowSystemNS::g_VKWindowSystemComponent->m_window);
}

void VKWindowSystemNS::hideMouseCursor()
{
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void VKWindowSystemNS::showMouseCursor()
{
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

INNO_SYSTEM_EXPORT ObjectStatus VKWindowSystem::getStatus()
{
	return VKWindowSystemNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT ButtonStatusMap VKWindowSystem::getButtonStatus()
{
	return windowCallbackWrapper::get().getButtonStatus();
}
