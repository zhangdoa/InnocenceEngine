#include "EventSystem.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

using ButtonCallbackMap = std::unordered_map<ButtonData, std::set<std::function<void()>*>, ButtonHasher>;
using MouseMovementCallbackMap = std::unordered_map<int, std::set<std::function<void(float)>*>>;

namespace InnoEventSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor);
	bool addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback);

	vec4 getMousePositionInWorldSpace();
	vec2 getMousePositionInScreenSpace();

	void buttonStatusCallback(ButtonData boundButton);
	void framebufferSizeCallback(int width, int height);
	void mousePositionCallback(float mouseXPos, float mouseYPos);
	void scrollCallback(float xoffset, float yoffset);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	const InputConfig m_inputConfig = { 256, 5 };

	ButtonStatusMap m_buttonStatus;
	ButtonCallbackMap m_buttonCallbacks;
	MouseMovementCallbackMap m_mouseMovementCallbacks;

	float m_mouseXOffset;
	float m_mouseYOffset;
	float m_mouseLastX;
	float m_mouseLastY;

	vec4 m_mousePositionInWorldSpace;
};

bool InnoEventSystemNS::setup()
{
	InnoEventSystemNS::m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoEventSystemNS::initialize()
{
	if (InnoEventSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		for (int i = 0; i < m_inputConfig.totalKeyCodes; i++)
		{
			m_buttonStatus.emplace(i, ButtonStatus::Released);
		}

		m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "EventSystem has been initialized.");

		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "EventSystem: Object is not created!");
		return false;
	}
}

bool InnoEventSystemNS::update()
{
	if (InnoEventSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		m_buttonStatus = g_pModuleManager->getWindowSystem()->getButtonStatus();

		for (auto& i : m_buttonStatus)
		{
			auto l_keybinding = m_buttonCallbacks.find(ButtonData{ i.first, i.second });
			if (l_keybinding != m_buttonCallbacks.end())
			{
				for (auto& j : l_keybinding->second)
				{
					if (j)
					{
						(*j)();
					}
				}
			}
		}

		if (m_mouseMovementCallbacks.size() != 0)
		{
			if (m_mouseXOffset != 0)
			{
				for (auto& j : m_mouseMovementCallbacks.find(0)->second)
				{
					(*j)(m_mouseXOffset);
				};
			}
			if (m_mouseYOffset != 0)
			{
				for (auto& j : m_mouseMovementCallbacks.find(1)->second)
				{
					(*j)(m_mouseYOffset);
				};
			}
			if (m_mouseXOffset != 0 || m_mouseYOffset != 0)
			{
				m_mouseXOffset = 0;
				m_mouseYOffset = 0;
			}
		}

		return true;
	}
	else
	{
		InnoEventSystemNS::m_objectStatus = ObjectStatus::Suspended;
		return false;
	}

	return true;
}

bool InnoEventSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "EventSystem has been terminated.");

	return true;
}

bool InnoEventSystemNS::addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor)
{
	auto l_result = m_buttonCallbacks.find(boundButton);
	if (l_result != m_buttonCallbacks.end())
	{
		l_result->second.emplace(buttonStatusCallbackFunctor);
	}
	else
	{
		m_buttonCallbacks.emplace(boundButton, std::set<std::function<void()>*>{buttonStatusCallbackFunctor});
	}

	return true;
}

bool InnoEventSystemNS::addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback)
{
	auto l_result = m_mouseMovementCallbacks.find(mouseCode);
	if (l_result != m_mouseMovementCallbacks.end())
	{
		l_result->second.emplace(mouseMovementCallback);
	}
	else
	{
		m_mouseMovementCallbacks.emplace(mouseCode, std::set<std::function<void(float)>*>{mouseMovementCallback});
	}

	return true;
}

vec4 InnoEventSystemNS::getMousePositionInWorldSpace()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	auto l_x = 2.0f * m_mouseLastX / l_screenResolution.x - 1.0f;
	auto l_y = 1.0f - 2.0f * m_mouseLastY / l_screenResolution.y;
	auto l_z = -1.0f;
	auto l_w = 1.0f;
	vec4 l_ndcSpace = vec4(l_x, l_y, l_z, l_w);

	auto l_cameraComponents = GetComponentManager(CameraComponent)->GetAllComponents();
	auto l_mainCamera = l_cameraComponents[0];
	auto pCamera = l_mainCamera->m_projectionMatrix;
	auto l_cameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);
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

vec2 InnoEventSystemNS::getMousePositionInScreenSpace()
{
	return vec2(m_mouseLastX, m_mouseLastY);
}

void InnoEventSystemNS::buttonStatusCallback(ButtonData boundButton)
{
	auto l_keybinding = m_buttonCallbacks.find(boundButton);
	if (l_keybinding != m_buttonCallbacks.end())
	{
		for (auto& j : l_keybinding->second)
		{
			if (j)
			{
				(*j)();
			}
		}
	}
}

void InnoEventSystemNS::framebufferSizeCallback(int width, int height)
{
	TVec2<unsigned int> l_newScreenResolution = TVec2<unsigned int>(width, height);
	g_pModuleManager->getRenderingFrontend()->setScreenResolution(l_newScreenResolution);
	g_pModuleManager->getRenderingServer()->Resize();
}

void InnoEventSystemNS::mousePositionCallback(float mouseXPos, float mouseYPos)
{
	m_mouseXOffset = mouseXPos - m_mouseLastX;
	m_mouseYOffset = m_mouseLastY - mouseYPos;

	m_mouseLastX = mouseXPos;
	m_mouseLastY = mouseYPos;
}

void InnoEventSystemNS::scrollCallback(float xoffset, float yoffset)
{
}

bool InnoEventSystem::setup()
{
	return InnoEventSystemNS::setup();
}

bool InnoEventSystem::initialize()
{
	return InnoEventSystemNS::initialize();
}

bool InnoEventSystem::update()
{
	return InnoEventSystemNS::update();
}

bool InnoEventSystem::terminate()
{
	return InnoEventSystemNS::terminate();
}

InputConfig InnoEventSystem::getInputConfig()
{
	return InnoEventSystemNS::m_inputConfig;
}

void InnoEventSystem::addButtonStatusCallback(ButtonData boundButton, std::function<void()>* buttonStatusCallbackFunctor)
{
	InnoEventSystemNS::addButtonStatusCallback(boundButton, buttonStatusCallbackFunctor);
}

void InnoEventSystem::addMouseMovementCallback(int mouseCode, std::function<void(float)>* mouseMovementCallback)
{
	InnoEventSystemNS::addMouseMovementCallback(mouseCode, mouseMovementCallback);
}

void InnoEventSystem::buttonStatusCallback(ButtonData boundButton)
{
	InnoEventSystemNS::buttonStatusCallback(boundButton);
}

void InnoEventSystem::framebufferSizeCallback(int width, int height)
{
	InnoEventSystemNS::framebufferSizeCallback(width, height);
}

void InnoEventSystem::mousePositionCallback(float mouseXPos, float mouseYPos)
{
	InnoEventSystemNS::mousePositionCallback(mouseXPos, mouseYPos);
}

void InnoEventSystem::scrollCallback(float xoffset, float yoffset)
{
}

vec4 InnoEventSystem::getMousePositionInWorldSpace()
{
	return InnoEventSystemNS::getMousePositionInWorldSpace();
}

vec2 InnoEventSystem::getMousePositionInScreenSpace()
{
	return InnoEventSystemNS::getMousePositionInScreenSpace();
}

ObjectStatus InnoEventSystem::getStatus()
{
	return InnoEventSystemNS::m_objectStatus;
}