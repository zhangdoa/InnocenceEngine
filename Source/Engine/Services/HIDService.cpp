#include "HIDService.h"
#include "../Common/LogService.h"
#include "RenderingConfigurationService.h"
#include "RenderingContextService.h"

#include "../Engine.h"
using namespace Inno;

bool HIDService::Setup(ISystemConfig* systemConfig)
{
	m_PreviousFrameButtonStates.reserve(m_InputConfig.totalKeyCodes);
	for (int i = 0; i < m_InputConfig.totalKeyCodes; i++)
	{
		m_PreviousFrameButtonStates.emplace_back(ButtonState(i, false));
	}

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool HIDService::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "HIDService has been initialized.");

		return true;
	}
	else
	{
		Log(Error, "HIDService is not created!");
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

	g_Engine->getWindowSystem()->ConsumeEvents([this](const std::vector<IWindowEvent*>& l_events)
		{
			for (auto l_event : l_events)
			{
				if (l_event->GetType() == WindowEventType::Button)
				{
					auto l_buttonState = static_cast<ButtonState*>(l_event);
					ButtonStateCallback(*l_buttonState);
				}
				else if (l_event->GetType() == WindowEventType::Mouse)
				{
					auto l_mouseState = static_cast<MouseState*>(l_event);
					MouseMovementCallback(*l_mouseState);
				}
			}
		});

	if (m_ButtonEvents.size() != 0)
	{
		for (auto& l_previousButtonState : m_PreviousFrameButtonStates)
		{
			auto l_pair = m_ButtonEvents.find(l_previousButtonState);
			if (l_pair != m_ButtonEvents.end())
			{
				for (auto& l_event : l_pair->second)
				{
					if (l_event.m_eventLifeTime != EventLifeTime::Continuous)
						continue;

					ExecuteEvent(l_event);
				}
			}
		}
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

	return true;
}

bool HIDService::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "HIDService has been terminated.");

	return true;
}

void HIDService::AddButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	auto l_result = m_ButtonEvents.find(buttonState);
	if (l_result != m_ButtonEvents.end())
		l_result->second.emplace(buttonEvent);
	else
		m_ButtonEvents.emplace(buttonState, std::set<ButtonEvent>{ buttonEvent });
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

void HIDService::ButtonStateCallback(const ButtonState& buttonState)
{
	auto& l_previousFrameButtonState = m_PreviousFrameButtonStates[buttonState.m_Code];
	auto l_result = m_ButtonEvents.find(buttonState);
	if (l_result != m_ButtonEvents.end())
	{
		for (auto& l_event : l_result->second)
		{
			if (l_event.m_eventLifeTime == EventLifeTime::Continuous)
				continue;

			if (l_previousFrameButtonState.m_isPressed != buttonState.m_isPressed)
				ExecuteEvent(l_event);
		}
	}

	l_previousFrameButtonState.m_isPressed = buttonState.m_isPressed;
}

void HIDService::WindowResizeCallback(int32_t width, int32_t height)
{
	m_IsResizing = true;
	TVec2<uint32_t> l_newScreenResolution = TVec2<uint32_t>(width, height);
	g_Engine->Get<RenderingConfigurationService>()->SetScreenResolution(l_newScreenResolution);
	g_Engine->getRenderingServer()->Resize();
	m_IsResizing = false;
}

void HIDService::MouseMovementCallback(const MouseState& mouseState)
{
	m_MouseXOffset = mouseState.m_X - m_MouseLastX;
	m_MouseYOffset = m_MouseLastY - mouseState.m_Y;

	m_MouseLastX = mouseState.m_X;
	m_MouseLastY = mouseState.m_Y;
}

bool HIDService::IsResizing()
{
	return m_IsResizing;
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