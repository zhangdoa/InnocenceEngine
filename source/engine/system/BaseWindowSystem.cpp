#include "BaseWindowSystem.h"

void BaseWindowSystem::setup()
{
}

void BaseWindowSystem::initialize()
{
	for (int i = 0; i < WindowSystemSingletonComponent::getInstance().NUM_KEYCODES; i++)
	{
		WindowSystemSingletonComponent::getInstance().m_buttonStatus.emplace(i, buttonStatus::RELEASED);
	}

	for (size_t i = 0; i < g_pGameSystem->getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addButtonStatusCallback(g_pGameSystem->getInputComponents()[i]->m_buttonStatusCallbackImpl);
		addMouseMovementCallback(g_pGameSystem->getInputComponents()[i]->m_mouseMovementCallbackImpl);
	}
}

void BaseWindowSystem::update()
{
	for (auto i : WindowSystemSingletonComponent::getInstance().m_buttonStatus)
	{
		auto l_keybinding = WindowSystemSingletonComponent::getInstance().m_buttonStatusCallback.find(button{ i.first, i.second });
		if (l_keybinding != WindowSystemSingletonComponent::getInstance().m_buttonStatusCallback.end())
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
	if (WindowSystemSingletonComponent::getInstance().m_mouseMovementCallback.size() != 0)
	{
		if (WindowSystemSingletonComponent::getInstance().m_mouseXOffset != 0)
		{
			for (auto j : WindowSystemSingletonComponent::getInstance().m_mouseMovementCallback.find(0)->second)
			{
				(*j)(WindowSystemSingletonComponent::getInstance().m_mouseXOffset);
			};
		}
		if (WindowSystemSingletonComponent::getInstance().m_mouseYOffset != 0)
		{
			for (auto j : WindowSystemSingletonComponent::getInstance().m_mouseMovementCallback.find(1)->second)
			{
				(*j)(WindowSystemSingletonComponent::getInstance().m_mouseYOffset);
			};
		}
		if (WindowSystemSingletonComponent::getInstance().m_mouseXOffset != 0 || WindowSystemSingletonComponent::getInstance().m_mouseYOffset != 0)
		{
			WindowSystemSingletonComponent::getInstance().m_mouseXOffset = 0;
			WindowSystemSingletonComponent::getInstance().m_mouseYOffset = 0;
		}
	}
}

void BaseWindowSystem::addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor)
{
	auto l_keyboardInputCallbackFunctionVector = WindowSystemSingletonComponent::getInstance().m_buttonStatusCallback.find(boundButton);
	if (l_keyboardInputCallbackFunctionVector != WindowSystemSingletonComponent::getInstance().m_buttonStatusCallback.end())
	{
		l_keyboardInputCallbackFunctionVector->second.emplace_back(buttonStatusCallbackFunctor);
	}
	else
	{
		WindowSystemSingletonComponent::getInstance().m_buttonStatusCallback.emplace(boundButton, std::vector<std::function<void()>*>{buttonStatusCallbackFunctor});
	}
}

void BaseWindowSystem::addButtonStatusCallback(button boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor)
{
	for (auto i : buttonStatusCallbackFunctor)
	{
		addButtonStatusCallback(boundButton, i);
	}
}

void BaseWindowSystem::addButtonStatusCallback(buttonStatusCallbackMap & buttonStatusCallbackFunctor)
{
	for (auto i : buttonStatusCallbackFunctor)
	{
		addButtonStatusCallback(i.first, i.second);
	}
}

void BaseWindowSystem::addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = WindowSystemSingletonComponent::getInstance().m_mouseMovementCallback.find(mouseCode);
	if (l_mouseMovementCallbackFunctionVector != WindowSystemSingletonComponent::getInstance().m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		WindowSystemSingletonComponent::getInstance().m_mouseMovementCallback.emplace(mouseCode, std::vector<std::function<void(double)>*>{mouseMovementCallback});
	}
}

void BaseWindowSystem::addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(mouseCode, i);
	}
}

void BaseWindowSystem::addMouseMovementCallback(mouseMovementCallbackMap& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
}

void BaseWindowSystem::framebufferSizeCallback(int width, int height)
{
	WindowSystemSingletonComponent::getInstance().m_windowResolution.x = width;
	WindowSystemSingletonComponent::getInstance().m_windowResolution.y = height;
}

void BaseWindowSystem::mousePositionCallback(double mouseXPos, double mouseYPos)
{
	WindowSystemSingletonComponent::getInstance().m_mouseXOffset = mouseXPos - WindowSystemSingletonComponent::getInstance().m_mouseLastX;
	WindowSystemSingletonComponent::getInstance().m_mouseYOffset = WindowSystemSingletonComponent::getInstance().m_mouseLastY - mouseYPos;

	WindowSystemSingletonComponent::getInstance().m_mouseLastX = mouseXPos;
	WindowSystemSingletonComponent::getInstance().m_mouseLastY = mouseYPos;
}

void BaseWindowSystem::scrollCallback(double xoffset, double yoffset)
{
	//@TODO: context based binding
	if (yoffset >= 0.0)
	{
		g_pGameSystem->getCameraComponents()[0]->m_FOV += 1.0;
	}
	else
	{
		g_pGameSystem->getCameraComponents()[0]->m_FOV -= 1.0;
	}
	//g_pPhysicsSystem->setupCameraComponentProjectionMatrix(g_pGameSystem->getCameraComponents()[0]);
}

vec4 BaseWindowSystem::calcMousePositionInWorldSpace()
{
	auto l_x = 2.0 * WindowSystemSingletonComponent::getInstance().m_mouseLastX / WindowSystemSingletonComponent::getInstance().m_windowResolution.x - 1.0;
	auto l_y = 1.0 - 2.0 * WindowSystemSingletonComponent::getInstance().m_mouseLastY / WindowSystemSingletonComponent::getInstance().m_windowResolution.y;
	auto l_z = -1.0;
	auto l_w = 1.0;
	vec4 l_ndcSpace = vec4(l_x, l_y, l_z, l_w);

	auto pCamera = g_pGameSystem->getCameraComponents()[0]->m_projectionMatrix;
	auto rCamera = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalRotMatrix();
	auto tCamera = g_pGameSystem->getTransformComponent(g_pGameSystem->getCameraComponents()[0]->getParentEntity())->m_transform.getInvertGlobalTranslationMatrix();
	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	l_ndcSpace = l_ndcSpace * pCamera.inverse();
	l_ndcSpace.z = -1.0;
	l_ndcSpace.w = 0.0;
	l_ndcSpace = l_ndcSpace * rCamera.inverse();
	l_ndcSpace = l_ndcSpace * tCamera.inverse();
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT

	l_ndcSpace = pCamera.inverse() * l_ndcSpace;
	l_ndcSpace.z = -1.0;
	l_ndcSpace.w = 0.0;
	l_ndcSpace = tCamera.inverse() * l_ndcSpace;
	l_ndcSpace = rCamera.inverse() * l_ndcSpace;
#endif
	l_ndcSpace = l_ndcSpace.normalize();
	return l_ndcSpace;
}

void BaseWindowSystem::swapBuffer()
{
}
