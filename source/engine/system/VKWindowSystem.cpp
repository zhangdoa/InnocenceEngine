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

	IInputSystem* m_inputSystem;

	static WindowSystemComponent* g_WindowSystemComponent;
	static VKWindowSystemComponent* g_VKWindowSystemComponent;

	bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow);

	void hideMouseCursor();
	void showMouseCursor();
}

bool VKWindowSystemNS::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	m_inputSystem = new InnoInputSystem();

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

	m_inputSystem->setup();

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
	windowCallbackWrapper::get().initialize(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, VKWindowSystemNS::m_inputSystem);

	//initialize input	
	VKWindowSystemNS::m_inputSystem->initialize();

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

		//update input
		//keyboard
		for (int i = 0; i < VKWindowSystemNS::g_WindowSystemComponent->NUM_KEYCODES; i++)
		{
			if (glfwGetKey(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, i) == GLFW_PRESS)
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::RELEASED;
				}
			}
		}
		//mouse
		for (int i = 0; i < VKWindowSystemNS::g_WindowSystemComponent->NUM_MOUSEBUTTONS; i++)
		{
			if (glfwGetMouseButton(VKWindowSystemNS::g_VKWindowSystemComponent->m_window, i) == GLFW_PRESS)
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::RELEASED;
				}
			}
		}
	}

	VKWindowSystemNS::m_inputSystem->update();
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