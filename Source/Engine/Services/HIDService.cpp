#include "HIDService.h"
#include "../Common/Logger.h"
#include "RenderingFrontend.h"

#include "../Engine.h"
using namespace Inno;
;

bool HIDService::Setup(ISystemConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool HIDService::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		g_Engine->Get<Logger>()->Log(LogLevel::Success, "EventSystem has been initialized.");

		return true;
	}
	else
	{
		g_Engine->Get<Logger>()->Log(LogLevel::Error, "EventSystem: Object is not created!");
		return false;
	}
}

bool HIDService::Update()
{
	if (HIDService::m_ObjectStatus != ObjectStatus::Activated)
	{
		HIDService::m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}

	auto l_buttonStates = g_Engine->getWindowSystem()->GetButtonState();
	for (auto& i : l_buttonStates)
	{
		ButtonStateCallback(i);
	}

	if (m_MouseMovementEvents.size() != 0)
	{
		if (m_MouseXOffset != 0)
		{
			for (auto& j : m_MouseMovementEvents.find(MouseMovementAxis::Horizontal)->second)
			{
				(*j.m_eventHandle)(m_MouseXOffset);
			}
		}
		if (m_MouseYOffset != 0)
		{
			for (auto& j : m_MouseMovementEvents.find(MouseMovementAxis::Vertical)->second)
			{
				(*j.m_eventHandle)(m_MouseYOffset);
			}
		}
		if (m_MouseXOffset != 0 || m_MouseYOffset != 0)
		{
			m_MouseXOffset = 0;
			m_MouseYOffset = 0;
		}
	}

	m_PreviousFrameButtonStates = l_buttonStates;

	return true;
}

bool HIDService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	g_Engine->Get<Logger>()->Log(LogLevel::Success, "EventSystem has been terminated.");

	return true;
}

void HIDService::AddButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	m_ButtonEvents.emplace(buttonState, buttonEvent);
}

void HIDService::AddMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent)
{
	auto l_result = m_MouseMovementEvents.find(mouseMovementAxis);
	if (l_result != m_MouseMovementEvents.end())
		l_result->second.emplace(mouseMovementEvent);
	else
		m_MouseMovementEvents.emplace(mouseMovementAxis, std::set<MouseMovementEvent>{ mouseMovementEvent });
}

Vec2 HIDService::GetMousePosition()
{
	return Vec2(m_MouseLastX, m_MouseLastY);
}

void HIDService::ButtonStateCallback(ButtonState buttonState)
{
	auto l_buttonEvents = m_ButtonEvents.equal_range(buttonState);
	auto l_resultCount = std::distance(l_buttonEvents.first, l_buttonEvents.second);
	if (!l_resultCount)
		return;

	for (auto it = l_buttonEvents.first; it != l_buttonEvents.second; it++)
	{
		if (it->second.m_eventLifeTime == EventLifeTime::Continuous && it->second.m_eventHandle)
			ExecuteEvent(it->second);
		else if (m_PreviousFrameButtonStates.size())
		{
			auto l_previousFrameButtonState = m_PreviousFrameButtonStates[it->first.m_code];
			if (l_previousFrameButtonState.m_isPressed != it->first.m_isPressed && it->second.m_eventHandle)
				ExecuteEvent(it->second);
		}
	}
}

void HIDService::WindowResizeCallback(int32_t width, int32_t height)
{
	TVec2<uint32_t> l_newScreenResolution = TVec2<uint32_t>(width, height);
	g_Engine->Get<RenderingFrontend>()->SetScreenResolution(l_newScreenResolution);
	g_Engine->getRenderingServer()->Resize();
}

void HIDService::MouseMovementCallback(float mouseXPos, float mouseYPos)
{
	m_MouseXOffset = mouseXPos - m_MouseLastX;
	m_MouseYOffset = m_MouseLastY - mouseYPos;

	m_MouseLastX = mouseXPos;
	m_MouseLastY = mouseYPos;
}

void HIDService::ExecuteEvent(const ButtonEvent& buttonEvent)
{
	auto l_event = reinterpret_cast<std::function<void()>*>(buttonEvent.m_eventHandle);
	(*l_event)();
}

InputConfig HIDService::GetInputConfig()
{
	return m_InputConfig;
}

ObjectStatus HIDService::GetStatus()
{
	return m_ObjectStatus;
}