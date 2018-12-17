#include "InputSystem.h"
#include "../component/WindowSystemComponent.h"
#include "../component/GameSystemComponent.h"

#include "ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoInputSystemNS
{
	vec4 calcMousePositionInWorldSpace();

	static WindowSystemComponent* g_WindowSystemComponent;
	static GameSystemComponent* g_GameSystemComponent;

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
};

INNO_SYSTEM_EXPORT void InnoInputSystem::setup()
{
	InnoInputSystemNS::g_WindowSystemComponent = &WindowSystemComponent::get();
	InnoInputSystemNS::g_GameSystemComponent = &GameSystemComponent::get();
}

INNO_SYSTEM_EXPORT void InnoInputSystem::initialize()
{
	for (int i = 0; i < InnoInputSystemNS::g_WindowSystemComponent->NUM_KEYCODES; i++)
	{
		InnoInputSystemNS::g_WindowSystemComponent->m_buttonStatus.emplace(i, ButtonStatus::RELEASED);
	}

	for (size_t i = 0; i < InnoInputSystemNS::g_GameSystemComponent->m_InputComponents.size(); i++)
	{
		// @TODO: multi input components need to register to multi map
		addButtonStatusCallback(InnoInputSystemNS::g_GameSystemComponent->m_InputComponents[i]->m_buttonStatusCallbackImpl);
		addMouseMovementCallback(InnoInputSystemNS::g_GameSystemComponent->m_InputComponents[i]->m_mouseMovementCallbackImpl);
	}

	InnoInputSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "InputSystem has been initialized.");
}

INNO_SYSTEM_EXPORT void InnoInputSystem::update()
{
	for (auto i : InnoInputSystemNS::g_WindowSystemComponent->m_buttonStatus)
	{
		auto l_keybinding = InnoInputSystemNS::g_WindowSystemComponent->m_buttonStatusCallback.find(ButtonData{ i.first, i.second });
		if (l_keybinding != InnoInputSystemNS::g_WindowSystemComponent->m_buttonStatusCallback.end())
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
	if (InnoInputSystemNS::g_WindowSystemComponent->m_mouseMovementCallback.size() != 0)
	{
		if (InnoInputSystemNS::g_WindowSystemComponent->m_mouseXOffset != 0)
		{
			for (auto j : InnoInputSystemNS::g_WindowSystemComponent->m_mouseMovementCallback.find(0)->second)
			{
				(*j)(InnoInputSystemNS::g_WindowSystemComponent->m_mouseXOffset);
			};
		}
		if (InnoInputSystemNS::g_WindowSystemComponent->m_mouseYOffset != 0)
		{
			for (auto j : InnoInputSystemNS::g_WindowSystemComponent->m_mouseMovementCallback.find(1)->second)
			{
				(*j)(InnoInputSystemNS::g_WindowSystemComponent->m_mouseYOffset);
			};
		}
		if (InnoInputSystemNS::g_WindowSystemComponent->m_mouseXOffset != 0 || InnoInputSystemNS::g_WindowSystemComponent->m_mouseYOffset != 0)
		{
			InnoInputSystemNS::g_WindowSystemComponent->m_mouseXOffset = 0;
			InnoInputSystemNS::g_WindowSystemComponent->m_mouseYOffset = 0;
		}
	}

	InnoInputSystemNS::g_WindowSystemComponent->m_mousePositionInWorldSpace = InnoInputSystemNS::calcMousePositionInWorldSpace();
}

INNO_SYSTEM_EXPORT void InnoInputSystem::terminate()
{
	InnoInputSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "InputSystem has been terminated.");
}

INNO_SYSTEM_EXPORT void InnoInputSystem::addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor)
{
	auto l_keyboardInputCallbackFunctionVector = InnoInputSystemNS::g_WindowSystemComponent->m_buttonStatusCallback.find(boundButton);
	if (l_keyboardInputCallbackFunctionVector != InnoInputSystemNS::g_WindowSystemComponent->m_buttonStatusCallback.end())
	{
		l_keyboardInputCallbackFunctionVector->second.emplace_back(buttonStatusCallbackFunctor);
	}
	else
	{
		InnoInputSystemNS::g_WindowSystemComponent->m_buttonStatusCallback.emplace(boundButton, std::vector<std::function<void()>*>{buttonStatusCallbackFunctor});
	}
}

INNO_SYSTEM_EXPORT void InnoInputSystem::addButtonStatusCallback(ButtonData boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor)
{
	for (auto i : buttonStatusCallbackFunctor)
	{
		addButtonStatusCallback(boundButton, i);
	}
}

INNO_SYSTEM_EXPORT void InnoInputSystem::addButtonStatusCallback(ButtonStatusCallbackMap & buttonStatusCallbackFunctor)
{
	for (auto i : buttonStatusCallbackFunctor)
	{
		addButtonStatusCallback(i.first, i.second);
	}
}

INNO_SYSTEM_EXPORT void InnoInputSystem::addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback)
{
	auto l_mouseMovementCallbackFunctionVector = InnoInputSystemNS::g_WindowSystemComponent->m_mouseMovementCallback.find(mouseCode);
	if (l_mouseMovementCallbackFunctionVector != InnoInputSystemNS::g_WindowSystemComponent->m_mouseMovementCallback.end())
	{
		l_mouseMovementCallbackFunctionVector->second.emplace_back(mouseMovementCallback);
	}
	else
	{
		InnoInputSystemNS::g_WindowSystemComponent->m_mouseMovementCallback.emplace(mouseCode, std::vector<std::function<void(float)>*>{mouseMovementCallback});
	}
}

INNO_SYSTEM_EXPORT void InnoInputSystem::addMouseMovementCallback(int mouseCode, std::vector<std::function<void(float)>*>& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(mouseCode, i);
	}
}

INNO_SYSTEM_EXPORT void InnoInputSystem::addMouseMovementCallback(MouseMovementCallbackMap& mouseMovementCallback)
{
	for (auto i : mouseMovementCallback)
	{
		addMouseMovementCallback(i.first, i.second);
	}
}

INNO_SYSTEM_EXPORT void InnoInputSystem::framebufferSizeCallback(int width, int height)
{
	InnoInputSystemNS::g_WindowSystemComponent->m_windowResolution.x = width;
	InnoInputSystemNS::g_WindowSystemComponent->m_windowResolution.y = height;
	g_pCoreSystem->getVisionSystem()->resize();
}

INNO_SYSTEM_EXPORT void InnoInputSystem::mousePositionCallback(float mouseXPos, float mouseYPos)
{
	InnoInputSystemNS::g_WindowSystemComponent->m_mouseXOffset = mouseXPos - InnoInputSystemNS::g_WindowSystemComponent->m_mouseLastX;
	InnoInputSystemNS::g_WindowSystemComponent->m_mouseYOffset = InnoInputSystemNS::g_WindowSystemComponent->m_mouseLastY - mouseYPos;

	InnoInputSystemNS::g_WindowSystemComponent->m_mouseLastX = mouseXPos;
	InnoInputSystemNS::g_WindowSystemComponent->m_mouseLastY = mouseYPos;
}

INNO_SYSTEM_EXPORT void InnoInputSystem::scrollCallback(float xoffset, float yoffset)
{
	//@TODO: context based binding
	if (yoffset >= 0.0f)
	{
		InnoInputSystemNS::g_GameSystemComponent->m_CameraComponents[0]->m_FOVX += 1.0f;
	}
	else
	{
		InnoInputSystemNS::g_GameSystemComponent->m_CameraComponents[0]->m_FOVX -= 1.0f;
	}
}

INNO_SYSTEM_EXPORT ObjectStatus InnoInputSystem::getStatus()
{
	return InnoInputSystemNS::m_objectStatus;
}

vec4 InnoInputSystemNS::calcMousePositionInWorldSpace()
{
	auto l_x = 2.0f * InnoInputSystemNS::g_WindowSystemComponent->m_mouseLastX / InnoInputSystemNS::g_WindowSystemComponent->m_windowResolution.x - 1.0f;
	auto l_y = 1.0f - 2.0f * InnoInputSystemNS::g_WindowSystemComponent->m_mouseLastY / InnoInputSystemNS::g_WindowSystemComponent->m_windowResolution.y;
	auto l_z = -1.0f;
	auto l_w = 1.0f;
	vec4 l_ndcSpace = vec4(l_x, l_y, l_z, l_w);

	auto l_mainCamera = InnoInputSystemNS::g_GameSystemComponent->m_CameraComponents[0];
	auto pCamera = l_mainCamera->m_projectionMatrix;
	auto l_cameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(l_mainCamera->m_parentEntity);
	auto rCamera =
		InnoMath::getInvertRotationMatrix(
			l_cameraTransformComponent->m_globalTransformVector.m_rot
		);
	auto tCamera =
		InnoMath::getInvertTranslationMatrix(
			l_cameraTransformComponent->m_globalTransformVector.m_pos
		);
	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	l_ndcSpace = InnoMath::mul(l_ndcSpace, pCamera.inverse());
	l_ndcSpace.z = -1.0f;
	l_ndcSpace.w = 0.0f;
	l_ndcSpace = InnoMath::mul(l_ndcSpace, rCamera.inverse());
	l_ndcSpace = InnoMath::mul(l_ndcSpace, tCamera.inverse());
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT

	l_ndcSpace = InnoMath::mul(pCamera.inverse(), l_ndcSpace);
	l_ndcSpace.z = -1.0f;
	l_ndcSpace.w = 0.0f;
	l_ndcSpace = InnoMath::mul(tCamera.inverse(), l_ndcSpace);
	l_ndcSpace = InnoMath::mul(rCamera.inverse(), l_ndcSpace);
#endif
	l_ndcSpace = l_ndcSpace.normalize();
	return l_ndcSpace;
}