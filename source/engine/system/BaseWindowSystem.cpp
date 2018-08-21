#include "BaseWindowSystem.h"

void BaseWindowSystem::addInputCallback()
{
	for (size_t i = 0; i < g_pGameSystem->getInputComponents().size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addKeyboardInputCallback(g_pGameSystem->getInputComponents()[i]->m_keyboardInputCallbackImpl);
		addMouseMovementCallback(g_pGameSystem->getInputComponents()[i]->m_mouseMovementCallbackImpl);
	}
}

void BaseWindowSystem::addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback)
{
	auto l_keyboardInputCallbackFunctionVector = WindowSystemSingletonComponent::getInstance().m_keyboardInputCallback.find(keyCode);
	if (l_keyboardInputCallbackFunctionVector != WindowSystemSingletonComponent::getInstance().m_keyboardInputCallback.end())
	{
		l_keyboardInputCallbackFunctionVector->second.emplace_back(keyboardInputCallback);
	}
	else
	{
		WindowSystemSingletonComponent::getInstance().m_keyboardInputCallback.emplace(keyCode, std::vector<std::function<void()>*>{keyboardInputCallback});
	}
}

void BaseWindowSystem::addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(keyCode, i);
	}
}

void BaseWindowSystem::addKeyboardInputCallback(std::unordered_map<int, std::vector<std::function<void()>*>>& keyboardInputCallback)
{
	for (auto i : keyboardInputCallback)
	{
		addKeyboardInputCallback(i.first, i.second);
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

void BaseWindowSystem::addMouseMovementCallback(std::unordered_map<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback)
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
