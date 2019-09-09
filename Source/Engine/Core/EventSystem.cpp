#include "EventSystem.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"
#include "../Core/InnoLogger.h"

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

using ButtonEventMap = std::unordered_multimap<ButtonState, ButtonEvent, ButtonStateHasher>;
using MouseMovementEventMap = std::unordered_map<int32_t, std::set<std::function<void(float)>*>>;

namespace InnoEventSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent);
	bool addMouseMovementCallback(int32_t mouseCode, std::function<void(float)>* mouseMovementCallback);

	Vec4 getMousePositionInWorldSpace();
	Vec2 getMousePositionInScreenSpace();

	void buttonStatusCallback(ButtonState buttonState);
	void framebufferSizeCallback(int32_t width, int32_t height);
	void mousePositionCallback(float mouseXPos, float mouseYPos);
	void scrollCallback(float xoffset, float yoffset);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	const InputConfig m_inputConfig = { 256, 5 };

	std::vector<ButtonState> m_previousFrameButtonStates;

	ButtonEventMap m_buttonEvents;
	MouseMovementEventMap m_mouseMovementEvents;

	float m_mouseXOffset;
	float m_mouseYOffset;
	float m_mouseLastX;
	float m_mouseLastY;

	Vec4 m_mousePositionInWorldSpace;
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
		m_objectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "EventSystem has been initialized.");

		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "EventSystem: Object is not created!");
		return false;
	}
}

bool InnoEventSystemNS::update()
{
	if (InnoEventSystemNS::m_objectStatus == ObjectStatus::Activated)
	{
		auto l_buttonStates = g_pModuleManager->getWindowSystem()->getButtonState();

		for (auto& i : l_buttonStates)
		{
			buttonStatusCallback(i);
		}

		if (m_mouseMovementEvents.size() != 0)
		{
			if (m_mouseXOffset != 0)
			{
				for (auto& j : m_mouseMovementEvents.find(0)->second)
				{
					(*j)(m_mouseXOffset);
				};
			}
			if (m_mouseYOffset != 0)
			{
				for (auto& j : m_mouseMovementEvents.find(1)->second)
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

		m_previousFrameButtonStates = l_buttonStates;

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
	InnoLogger::Log(LogLevel::Success, "EventSystem has been terminated.");

	return true;
}

bool InnoEventSystemNS::addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	m_buttonEvents.emplace(buttonState, buttonEvent);

	return true;
}

bool InnoEventSystemNS::addMouseMovementCallback(int32_t mouseCode, std::function<void(float)>* mouseMovementCallback)
{
	auto l_result = m_mouseMovementEvents.find(mouseCode);
	if (l_result != m_mouseMovementEvents.end())
	{
		l_result->second.emplace(mouseMovementCallback);
	}
	else
	{
		m_mouseMovementEvents.emplace(mouseCode, std::set<std::function<void(float)>*>{mouseMovementCallback});
	}

	return true;
}

Vec4 InnoEventSystemNS::getMousePositionInWorldSpace()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	auto l_x = 2.0f * m_mouseLastX / l_screenResolution.x - 1.0f;
	auto l_y = 1.0f - 2.0f * m_mouseLastY / l_screenResolution.y;
	auto l_z = -1.0f;
	auto l_w = 1.0f;
	Vec4 l_ndcSpace = Vec4(l_x, l_y, l_z, l_w);

	auto l_mainCamera = GetComponentManager(CameraComponent)->GetMainCamera();
	if (l_mainCamera == nullptr)
	{
		return Vec4();
	}
	auto l_cameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);
	if (l_cameraTransformComponent == nullptr)
	{
		return Vec4();
	}
	auto pCamera = l_mainCamera->m_projectionMatrix;
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

Vec2 InnoEventSystemNS::getMousePositionInScreenSpace()
{
	return Vec2(m_mouseLastX, m_mouseLastY);
}

void InnoEventSystemNS::buttonStatusCallback(ButtonState buttonState)
{
	auto l_buttonEvents = m_buttonEvents.equal_range(buttonState);
	auto l_resultCount = std::distance(l_buttonEvents.first, l_buttonEvents.second);

	if (l_resultCount)
	{
		for (auto it = l_buttonEvents.first; it != l_buttonEvents.second; it++)
		{
			if (it->second.m_eventLifeTime == EventLifeTime::Continuous)
			{
				if (it->second.m_eventHandle)
				{
					auto l_event = reinterpret_cast<std::function<void()>*>(it->second.m_eventHandle);
					(*l_event)();
				}
			}
			else
			{
				auto l_previousFrameButtonState = m_previousFrameButtonStates[it->first.m_code];
				if (l_previousFrameButtonState.m_isPressed != it->first.m_isPressed)
				{
					if (it->second.m_eventHandle)
					{
						auto l_event = reinterpret_cast<std::function<void()>*>(it->second.m_eventHandle);
						(*l_event)();
					}
				}
			}
		}
	}
}

void InnoEventSystemNS::framebufferSizeCallback(int32_t width, int32_t height)
{
	TVec2<uint32_t> l_newScreenResolution = TVec2<uint32_t>(width, height);
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

void InnoEventSystem::addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	InnoEventSystemNS::addButtonStateCallback(buttonState, buttonEvent);
}

void InnoEventSystem::addMouseMovementCallback(int32_t mouseCode, std::function<void(float)>* mouseMovementCallback)
{
	InnoEventSystemNS::addMouseMovementCallback(mouseCode, mouseMovementCallback);
}

void InnoEventSystem::buttonStatusCallback(ButtonState buttonState)
{
	InnoEventSystemNS::buttonStatusCallback(buttonState);
}

void InnoEventSystem::framebufferSizeCallback(int32_t width, int32_t height)
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

Vec4 InnoEventSystem::getMousePositionInWorldSpace()
{
	return InnoEventSystemNS::getMousePositionInWorldSpace();
}

Vec2 InnoEventSystem::getMousePositionInScreenSpace()
{
	return InnoEventSystemNS::getMousePositionInScreenSpace();
}

ObjectStatus InnoEventSystem::getStatus()
{
	return InnoEventSystemNS::m_objectStatus;
}