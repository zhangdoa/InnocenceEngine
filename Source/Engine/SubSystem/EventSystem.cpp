#include "EventSystem.h"
#include "../Common/CommonMacro.inl"
#include "../Core/InnoLogger.h"

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

using ButtonEventMap = std::unordered_multimap<ButtonState, ButtonEvent, ButtonStateHasher>;
using MouseMovementEventMap = std::unordered_map<MouseMovementAxis, std::set<MouseMovementEvent>>;

namespace InnoEventSystemNS
{
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent);
	bool addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent);

	Vec2 getMousePosition();

	void buttonStateCallback(ButtonState buttonState);
	void windowSizeCallback(int32_t width, int32_t height);
	void mouseMovementCallback(float mouseXPos, float mouseYPos);
	void scrollCallback(float xoffset, float yoffset);

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

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
	InnoEventSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoEventSystemNS::initialize()
{
	if (InnoEventSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
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
	if (InnoEventSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		auto l_buttonStates = g_pModuleManager->getWindowSystem()->getButtonState();

		for (auto& i : l_buttonStates)
		{
			buttonStateCallback(i);
		}

		if (m_mouseMovementEvents.size() != 0)
		{
			if (m_mouseXOffset != 0)
			{
				for (auto& j : m_mouseMovementEvents.find(MouseMovementAxis::Horizontal)->second)
				{
					(*j.m_eventHandle)(m_mouseXOffset);
				};
			}
			if (m_mouseYOffset != 0)
			{
				for (auto& j : m_mouseMovementEvents.find(MouseMovementAxis::Vertical)->second)
				{
					(*j.m_eventHandle)(m_mouseYOffset);
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
		InnoEventSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	return true;
}

bool InnoEventSystemNS::terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "EventSystem has been terminated.");

	return true;
}

bool InnoEventSystemNS::addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	m_buttonEvents.emplace(buttonState, buttonEvent);

	return true;
}

bool InnoEventSystemNS::addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent)
{
	auto l_result = m_mouseMovementEvents.find(mouseMovementAxis);
	if (l_result != m_mouseMovementEvents.end())
	{
		l_result->second.emplace(mouseMovementEvent);
	}
	else
	{
		m_mouseMovementEvents.emplace(mouseMovementAxis, std::set<MouseMovementEvent>{ mouseMovementEvent });
	}

	return true;
}

Vec2 InnoEventSystemNS::getMousePosition()
{
	return Vec2(m_mouseLastX, m_mouseLastY);
}

void InnoEventSystemNS::buttonStateCallback(ButtonState buttonState)
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

void InnoEventSystemNS::windowSizeCallback(int32_t width, int32_t height)
{
	TVec2<uint32_t> l_newScreenResolution = TVec2<uint32_t>(width, height);
	g_pModuleManager->getRenderingFrontend()->setScreenResolution(l_newScreenResolution);
	g_pModuleManager->getRenderingServer()->Resize();
}

void InnoEventSystemNS::mouseMovementCallback(float mouseXPos, float mouseYPos)
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

void InnoEventSystem::addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent)
{
	InnoEventSystemNS::addMouseMovementCallback(mouseMovementAxis, mouseMovementEvent);
}

void InnoEventSystem::buttonStateCallback(ButtonState buttonState)
{
	InnoEventSystemNS::buttonStateCallback(buttonState);
}

void InnoEventSystem::windowSizeCallback(int32_t width, int32_t height)
{
	InnoEventSystemNS::windowSizeCallback(width, height);
}

void InnoEventSystem::mouseMovementCallback(float mouseXPos, float mouseYPos)
{
	InnoEventSystemNS::mouseMovementCallback(mouseXPos, mouseYPos);
}

void InnoEventSystem::scrollCallback(float xoffset, float yoffset)
{
	InnoEventSystemNS::scrollCallback(xoffset, yoffset);
}

Vec2 InnoEventSystem::getMousePosition()
{
	return InnoEventSystemNS::getMousePosition();
}

ObjectStatus InnoEventSystem::getStatus()
{
	return InnoEventSystemNS::m_ObjectStatus;
}