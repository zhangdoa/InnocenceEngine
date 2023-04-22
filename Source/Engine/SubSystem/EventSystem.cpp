#include "EventSystem.h"
#include "../Core/Logger.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

using ButtonEventMap = std::unordered_multimap<ButtonState, ButtonEvent, ButtonStateHasher>;
using MouseMovementEventMap = std::unordered_map<MouseMovementAxis, std::set<MouseMovementEvent>>;

namespace EventSystemNS
{
	bool Setup();
	bool Initialize();
	bool Update();
	bool Terminate();

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

bool EventSystemNS::Setup()
{
	EventSystemNS::m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool EventSystemNS::Initialize()
{
	if (EventSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		Logger::Log(LogLevel::Success, "EventSystem has been initialized.");

		return true;
	}
	else
	{
		Logger::Log(LogLevel::Error, "EventSystem: Object is not created!");
		return false;
	}
}

bool EventSystemNS::Update()
{
	if (EventSystemNS::m_ObjectStatus == ObjectStatus::Activated)
	{
		auto l_buttonStates = g_Engine->getWindowSystem()->getButtonState();

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
		EventSystemNS::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	return true;
}

bool EventSystemNS::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "EventSystem has been terminated.");

	return true;
}

bool EventSystemNS::addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	m_buttonEvents.emplace(buttonState, buttonEvent);

	return true;
}

bool EventSystemNS::addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent)
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

Vec2 EventSystemNS::getMousePosition()
{
	return Vec2(m_mouseLastX, m_mouseLastY);
}

void EventSystemNS::buttonStateCallback(ButtonState buttonState)
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
				if (m_previousFrameButtonStates.size())
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
}

void EventSystemNS::windowSizeCallback(int32_t width, int32_t height)
{
	TVec2<uint32_t> l_newScreenResolution = TVec2<uint32_t>(width, height);
	g_Engine->getRenderingFrontend()->SetScreenResolution(l_newScreenResolution);
	g_Engine->getRenderingServer()->Resize();
}

void EventSystemNS::mouseMovementCallback(float mouseXPos, float mouseYPos)
{
	m_mouseXOffset = mouseXPos - m_mouseLastX;
	m_mouseYOffset = m_mouseLastY - mouseYPos;

	m_mouseLastX = mouseXPos;
	m_mouseLastY = mouseYPos;
}

void EventSystemNS::scrollCallback(float xoffset, float yoffset)
{
}

bool EventSystem::Setup(ISystemConfig* systemConfig)
{
	return EventSystemNS::Setup();
}

bool EventSystem::Initialize()
{
	return EventSystemNS::Initialize();
}

bool EventSystem::Update()
{
	return EventSystemNS::Update();
}

bool EventSystem::Terminate()
{
	return EventSystemNS::Terminate();
}

InputConfig EventSystem::getInputConfig()
{
	return EventSystemNS::m_inputConfig;
}

void EventSystem::addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	EventSystemNS::addButtonStateCallback(buttonState, buttonEvent);
}

void EventSystem::addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent)
{
	EventSystemNS::addMouseMovementCallback(mouseMovementAxis, mouseMovementEvent);
}

void EventSystem::buttonStateCallback(ButtonState buttonState)
{
	EventSystemNS::buttonStateCallback(buttonState);
}

void EventSystem::windowSizeCallback(int32_t width, int32_t height)
{
	EventSystemNS::windowSizeCallback(width, height);
}

void EventSystem::mouseMovementCallback(float mouseXPos, float mouseYPos)
{
	EventSystemNS::mouseMovementCallback(mouseXPos, mouseYPos);
}

void EventSystem::scrollCallback(float xoffset, float yoffset)
{
	EventSystemNS::scrollCallback(xoffset, yoffset);
}

Vec2 EventSystem::getMousePosition()
{
	return EventSystemNS::getMousePosition();
}

ObjectStatus EventSystem::GetStatus()
{
	return EventSystemNS::m_ObjectStatus;
}