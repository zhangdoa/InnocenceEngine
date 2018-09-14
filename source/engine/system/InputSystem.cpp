#include "InputSystem.h"
#include "GameSystem.h"
#include "LogSystem.h" 

namespace InnoInputSystem
{
	objectStatus m_InputSystemStatus = objectStatus::SHUTDOWN;
};

void InnoInputSystem::setup()
{
	m_InputSystemStatus = objectStatus::ALIVE;
}

void InnoInputSystem::initialize()
{
	for (int i = 0; i < WindowSystemSingletonComponent::getInstance().NUM_KEYCODES; i++)
	{
		WindowSystemSingletonComponent::getInstance().m_buttonStatus.emplace(i, buttonStatus::RELEASED);
	}

	for (size_t i = 0; i < InnoGameSystem::getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addButtonStatusCallback(InnoGameSystem::getInputComponents()[i]->m_buttonStatusCallbackImpl);
		addMouseMovementCallback(InnoGameSystem::getInputComponents()[i]->m_mouseMovementCallbackImpl);
	}
	InnoLogSystem::printLog("InputSystem has been initialized.");
}

void InnoInputSystem::update()
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

void InnoInputSystem::shutdown()
{
	m_InputSystemStatus = objectStatus::SHUTDOWN;
	InnoLogSystem::printLog("InputSystem has been shutdown.");
}

void InnoInputSystem::addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor)
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
	WindowSystemSingletonComponent::getInstance().m_windowResolution.x = width;
	WindowSystemSingletonComponent::getInstance().m_windowResolution.y = height;
}

void InnoInputSystem::mousePositionCallback(double mouseXPos, double mouseYPos)
{
	WindowSystemSingletonComponent::getInstance().m_mouseXOffset = mouseXPos - WindowSystemSingletonComponent::getInstance().m_mouseLastX;
	WindowSystemSingletonComponent::getInstance().m_mouseYOffset = WindowSystemSingletonComponent::getInstance().m_mouseLastY - mouseYPos;

	WindowSystemSingletonComponent::getInstance().m_mouseLastX = mouseXPos;
	WindowSystemSingletonComponent::getInstance().m_mouseLastY = mouseYPos;
}

void InnoInputSystem::scrollCallback(double xoffset, double yoffset)
{
	//@TODO: context based binding
	if (yoffset >= 0.0)
	{
		InnoGameSystem::getCameraComponents()[0]->m_FOVX += 1.0;
	}
	else
	{
		InnoGameSystem::getCameraComponents()[0]->m_FOVX -= 1.0;
	}
	//InnoPhysicsSystem::setupCameraComponentProjectionMatrix(InnoGameSystem::getCameraComponents()[0]);
}

objectStatus InnoInputSystem::getStatus()
{
	return m_InputSystemStatus;
}

vec4 InnoInputSystem::calcMousePositionInWorldSpace()
{
	auto l_x = 2.0 * WindowSystemSingletonComponent::getInstance().m_mouseLastX / WindowSystemSingletonComponent::getInstance().m_windowResolution.x - 1.0;
	auto l_y = 1.0 - 2.0 * WindowSystemSingletonComponent::getInstance().m_mouseLastY / WindowSystemSingletonComponent::getInstance().m_windowResolution.y;
	auto l_z = -1.0;
	auto l_w = 1.0;
	vec4 l_ndcSpace = vec4(l_x, l_y, l_z, l_w);

	auto pCamera = InnoGameSystem::getCameraComponents()[0]->m_projectionMatrix;
	auto rCamera = InnoGameSystem::getTransformComponent(InnoGameSystem::getCameraComponents()[0]->m_parentEntity)->m_transform.getInvertGlobalRotationMatrix();
	auto tCamera = InnoGameSystem::getTransformComponent(InnoGameSystem::getCameraComponents()[0]->m_parentEntity)->m_transform.getInvertGlobalTranslationMatrix();
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