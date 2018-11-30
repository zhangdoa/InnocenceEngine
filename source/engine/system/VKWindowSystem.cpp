#include "VKWindowSystem.h"

#include "../component/WindowSystemSingletonComponent.h"
#include "../component/VKWindowSystemSingletonComponent.h"
#include "GLFWWrapper.h"

#include "InputSystem.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE VKWindowSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	IInputSystem* m_inputSystem;

	static WindowSystemSingletonComponent* g_WindowSystemSingletonComponent;
	static VKWindowSystemSingletonComponent* g_VKWindowSystemSingletonComponent;

	void hideMouseCursor();
	void showMouseCursor();
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow)
{
	VKWindowSystemNS::m_inputSystem = new InnoInputSystem();

	VKWindowSystemNS::g_WindowSystemSingletonComponent = &WindowSystemSingletonComponent::getInstance();
	VKWindowSystemNS::g_VKWindowSystemSingletonComponent = &VKWindowSystemSingletonComponent::getInstance();

	VKWindowSystemNS::g_WindowSystemSingletonComponent->m_windowName = g_pCoreSystem->getGameSystem()->getGameName();

	//setup window
	if (glfwInit() != GL_TRUE)
	{
		VKWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to initialize GLFW.");
		return false;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	// Open a window
	VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window = glfwCreateWindow((int)VKWindowSystemNS::g_WindowSystemSingletonComponent->m_windowResolution.x, (int)VKWindowSystemNS::g_WindowSystemSingletonComponent->m_windowResolution.y, VKWindowSystemNS::g_WindowSystemSingletonComponent->m_windowName.c_str(), NULL, NULL);
	glfwMakeContextCurrent(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window);
	if (VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window == nullptr) {
		VKWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "VKWindowSystem: Failed to open GLFW window.");
		glfwTerminate();
		return false;
	}

	//setup input
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window, GLFW_STICKY_KEYS, GL_TRUE);

	VKWindowSystemNS::m_inputSystem->setup();

	VKWindowSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::initialize()
{
	//initialize window
	windowCallbackWrapper::getInstance().initialize(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window, VKWindowSystemNS::m_inputSystem);

	//initialize input	
	VKWindowSystemNS::m_inputSystem->initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKWindowSystem has been initialized.");
	return true;
}

INNO_SYSTEM_EXPORT bool VKWindowSystem::update()
{
	//update window
	if (VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window == nullptr || glfwWindowShouldClose(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window) != 0)
	{
		VKWindowSystemNS::m_objectStatus = ObjectStatus::STANDBY;
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "VKWindowSystem: Input error or Window closed.");
	}
	else
	{
		glfwPollEvents();

		//update input
		//keyboard
		for (int i = 0; i < VKWindowSystemNS::g_WindowSystemSingletonComponent->NUM_KEYCODES; i++)
		{
			if (glfwGetKey(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window, i) == GLFW_PRESS)
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::RELEASED;
				}
			}
		}
		//mouse
		for (int i = 0; i < VKWindowSystemNS::g_WindowSystemSingletonComponent->NUM_MOUSEBUTTONS; i++)
		{
			if (glfwGetMouseButton(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window, i) == GLFW_PRESS)
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.end())
				{
					l_result->second = ButtonStatus::PRESSED;
				}
			}
			else
			{
				auto l_result = VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.find(i);
				if (l_result != VKWindowSystemNS::g_WindowSystemSingletonComponent->m_buttonStatus.end())
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
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window, GLFW_STICKY_KEYS, GL_FALSE);
	glfwDestroyWindow(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window);
	glfwTerminate();

	VKWindowSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VKWindowSystem has been terminated.");
	return true;
}

void VKWindowSystem::swapBuffer()
{
	glfwSwapBuffers(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window);
}

void VKWindowSystemNS::hideMouseCursor()
{
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void VKWindowSystemNS::showMouseCursor()
{
	glfwSetInputMode(VKWindowSystemNS::g_VKWindowSystemSingletonComponent->m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

INNO_SYSTEM_EXPORT ObjectStatus VKWindowSystem::getStatus()
{
	return VKWindowSystemNS::m_objectStatus;
}