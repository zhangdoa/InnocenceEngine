#include "InputSystem.h"
#include "../LowLevelSystem/LogSystem.h"
#include "../HighLevelSystem/GameSystem.h"
#include "../../component/WindowSystemSingletonComponent.h"
#include "../../component/GameSystemSingletonComponent.h"

namespace InnoInputSystem
{
	vec4 calcMousePositionInWorldSpace();

	static WindowSystemSingletonComponent* g_WindowSystemSingletonComponent;
	static GameSystemSingletonComponent* g_GameSystemSingletonComponent;

	objectStatus m_InputSystemStatus = objectStatus::SHUTDOWN;
};

void InnoInputSystem::setup()
{
	g_WindowSystemSingletonComponent = &WindowSystemSingletonComponent::getInstance();
	g_GameSystemSingletonComponent = &GameSystemSingletonComponent::getInstance();

	m_InputSystemStatus = objectStatus::ALIVE;
}

void InnoInputSystem::initialize()
{
	for (int i = 0; i < g_WindowSystemSingletonComponent->NUM_KEYCODES; i++)
	{
		g_WindowSystemSingletonComponent->m_buttonStatus.emplace(i, buttonStatus::RELEASED);
	}

	for (size_t i = 0; i < g_GameSystemSingletonComponent->m_inputComponents.size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addButtonStatusCallback(g_GameSystemSingletonComponent->m_inputComponents[i]->m_buttonStatusCallbackImpl);
		addMouseMovementCallback(g_GameSystemSingletonComponent->m_inputComponents[i]->m_mouseMovementCallbackImpl);
	}
	InnoLogSystem::printLog("InputSystem has been initialized.");
}

void InnoInputSystem::update()
{
	for (auto i : g_WindowSystemSingletonComponent->m_buttonStatus)
	{
		auto l_keybinding = g_WindowSystemSingletonComponent->m_buttonStatusCallback.find(button{ i.first, i.second });
		if (l_keybinding != g_WindowSystemSingletonComponent->m_buttonStatusCallback.end())
		{
			for (auto j : l_keybinding->second)
			{
				if (j)
				{
					(*j)();
				}
			}
		}
	}
	// @TODO: relative offset for editor window
	if (g_WindowSystemSingletonComponent->m_mouseMovementCallback.size() != 0)
	{
		if (g_WindowSystemSingletonComponent->m_mouseXOffset != 0)
		{
			for (auto j : g_WindowSystemSingletonComponent->m_mouseMovementCallback.find(0)->second)
			{
				(*j)(g_WindowSystemSingletonComponent->m_mouseXOffset);
			};
		}
		if (g_WindowSystemSingletonComponent->m_mouseYOffset != 0)
		{
			for (auto j : g_WindowSystemSingletonComponent->m_mouseMovementCallback.find(1)->second)
			{
				(*j)(g_WindowSystemSingletonComponent->m_mouseYOffset);
			};
		}
		if (g_WindowSystemSingletonComponent->m_mouseXOffset != 0 || g_WindowSystemSingletonComponent->m_mouseYOffset != 0)
		{
			g_WindowSystemSingletonComponent->m_mouseXOffset = 0;
			g_WindowSystemSingletonComponent->m_mouseYOffset = 0;
		}
	}

	g_WindowSystemSingletonComponent->m_mousePositionInWorldSpace = calcMousePositionInWorldSpace();
}

void InnoInputSystem::terminate()
{
	m_InputSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("InputSystem has been terminated.");
}

void InnoInputSystem::addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor)
{
	auto l_keyboardInputCallbackFunctionVector = g_WindowSystemSingletonComponent->m_buttonStatusCallback.find(boundButton);
	if (l_keyboardInputCallbackFunctionVector != g_WindowSystemSingletonComponent->m_buttonStatusCallback.end())
	{
		l_keyboardInputCallbackFunctionVector->second.emplace_back(buttonStatusCallbackFunctor);
	}
	else
	{
		g_WindowSystemSingletonComponent->m_buttonStatusCallback.emplace(boundButton, std::vector<std::function<void()>*>{buttonStatusCallbackFunctor});
	}
}

void InnoInputSystem::addButtonStatusCallback(button boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor)
{
	for (auto i : buttonStatusCallbackFunctor)
	{
		addButtonStatusCallback(boundButton, i);
	}
}

void InnoInputSystem::addButtonStatusCallback(buttonStatusCallbackMap & buttonStatusCallbackFunctor)
{
	for (auto i : buttonStatusCallbackFunctor)
	{
		addButtonStatusCallback(i.first, i.second);
	}
}

void InnoInputSystem::addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = g_WindowSystemSingletonComponent->m_mouseMovementCallback.find(mouseCode);
	if (l_mouseMovementCallbackFunctionVector != g_WindowSystemSingletonComponent->m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		g_WindowSystemSingletonComponent->m_mouseMovementCallback.emplace(mouseCode, std::vector<std::function<void(double)>*>{mouseMovementCallback});
	}
}

void InnoInputSystem::addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(mouseCode, i);
	}
}

void InnoInputSystem::addMouseMovementCallback(mouseMovementCallbackMap& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
}

void InnoInputSystem::framebufferSizeCallback(int width, int height)
{
	g_WindowSystemSingletonComponent->m_windowResolution.x = width;
	g_WindowSystemSingletonComponent->m_windowResolution.y = height;
}

void InnoInputSystem::mousePositionCallback(double mouseXPos, double mouseYPos)
{
	g_WindowSystemSingletonComponent->m_mouseXOffset = mouseXPos - g_WindowSystemSingletonComponent->m_mouseLastX;
	g_WindowSystemSingletonComponent->m_mouseYOffset = g_WindowSystemSingletonComponent->m_mouseLastY - mouseYPos;

	g_WindowSystemSingletonComponent->m_mouseLastX = mouseXPos;
	g_WindowSystemSingletonComponent->m_mouseLastY = mouseYPos;
}

void InnoInputSystem::scrollCallback(double xoffset, double yoffset)
{
	//@TODO: context based binding
	if (yoffset >= 0.0)
	{
		g_GameSystemSingletonComponent->m_cameraComponents[0]->m_FOVX += 1.0;
	}
	else
	{
		g_GameSystemSingletonComponent->m_cameraComponents[0]->m_FOVX -= 1.0;
	}
	//InnoPhysicsSystem::setupCameraComponentProjectionMatrix(InnoGameSystem::getCameraComponents()[0]);
}

objectStatus InnoInputSystem::getStatus()
{
	return m_InputSystemStatus;
}

vec4 InnoInputSystem::calcMousePositionInWorldSpace()
{
	auto l_x = 2.0 * g_WindowSystemSingletonComponent->m_mouseLastX / g_WindowSystemSingletonComponent->m_windowResolution.x - 1.0;
	auto l_y = 1.0 - 2.0 * g_WindowSystemSingletonComponent->m_mouseLastY / g_WindowSystemSingletonComponent->m_windowResolution.y;
	auto l_z = -1.0;
	auto l_w = 1.0;
	vec4 l_ndcSpace = vec4(l_x, l_y, l_z, l_w);

	auto l_mainCamera = g_GameSystemSingletonComponent->m_cameraComponents[0];
	auto pCamera = l_mainCamera->m_projectionMatrix;
	auto rCamera = InnoMath::getInvertRotationMatrix(InnoGameSystem::getTransformComponent(l_mainCamera->m_parentEntity)->m_globalTransformVector.m_rot);
	auto tCamera = InnoMath::getInvertTranslationMatrix(InnoGameSystem::getTransformComponent(l_mainCamera->m_parentEntity)->m_globalTransformVector.m_pos);
	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	l_ndcSpace = InnoMath::mul(l_ndcSpace, pCamera.inverse());
	l_ndcSpace.z = -1.0;
	l_ndcSpace.w = 0.0;
	l_ndcSpace = InnoMath::mul(l_ndcSpace, rCamera.inverse());
	l_ndcSpace = InnoMath::mul(l_ndcSpace, tCamera.inverse());
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT

	l_ndcSpace = InnoMath::mul(pCamera.inverse(), l_ndcSpace);
	l_ndcSpace.z = -1.0;
	l_ndcSpace.w = 0.0;
	l_ndcSpace = InnoMath::mul(tCamera.inverse(), l_ndcSpace);
	l_ndcSpace = InnoMath::mul(rCamera.inverse(), l_ndcSpace);
#endif
	l_ndcSpace = l_ndcSpace.normalize();
	return l_ndcSpace;
}