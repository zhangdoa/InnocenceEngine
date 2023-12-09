#include "EventSystem.h"
#include "../Core/Logger.h"

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

using ButtonEventMap = std::unordered_multimap<ButtonState, ButtonEvent, ButtonStateHasher>;
using MouseMovementEventMap = std::unordered_map<MouseMovementAxis, std::set<MouseMovementEvent>>;

namespace Inno
{
	class EventSystemImpl
	{
	public:
		bool Setup();
		bool Initialize();
		bool Update();
		bool Terminate();

		bool AddButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent);
		bool AddMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent);

		Vec2 GetMousePosition();

		void ButtonStateCallback(ButtonState buttonState);
		void WindowResizeCallback(int32_t width, int32_t height);
		void MouseMovementCallback(float mouseXPos, float mouseYPos);
		void MouseScrollCallback(float xOffset, float yOffset);

		void ExecuteEvent(const ButtonEvent& buttonEvent);

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		const InputConfig m_InputConfig = { 256, 5 };

		std::vector<ButtonState> m_PreviousFrameButtonStates;

		ButtonEventMap m_ButtonEvents;
		MouseMovementEventMap m_MouseMovementEvents;

		float m_MouseXOffset;
		float m_MouseYOffset;
		float m_MouseLastX;
		float m_MouseLastY;

		Vec4 m_MousePositionInWorldSpace;
	};
}

bool EventSystemImpl::Setup()
{
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool EventSystemImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
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

bool EventSystemImpl::Update()
{
	if (EventSystemImpl::m_ObjectStatus != ObjectStatus::Activated)
	{
		EventSystemImpl::m_ObjectStatus = ObjectStatus::Suspended;
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

bool EventSystemImpl::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Logger::Log(LogLevel::Success, "EventSystem has been terminated.");

	return true;
}

bool EventSystemImpl::AddButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	m_ButtonEvents.emplace(buttonState, buttonEvent);

	return true;
}

bool EventSystemImpl::AddMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent)
{
	auto l_result = m_MouseMovementEvents.find(mouseMovementAxis);
	if (l_result != m_MouseMovementEvents.end())
		l_result->second.emplace(mouseMovementEvent);
	else
		m_MouseMovementEvents.emplace(mouseMovementAxis, std::set<MouseMovementEvent>{ mouseMovementEvent });

	return true;
}

Vec2 EventSystemImpl::GetMousePosition()
{
	return Vec2(m_MouseLastX, m_MouseLastY);
}

void EventSystemImpl::ButtonStateCallback(ButtonState buttonState)
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

void EventSystemImpl::WindowResizeCallback(int32_t width, int32_t height)
{
	TVec2<uint32_t> l_newScreenResolution = TVec2<uint32_t>(width, height);
	g_Engine->getRenderingFrontend()->SetScreenResolution(l_newScreenResolution);
	g_Engine->getRenderingServer()->Resize();
}

void EventSystemImpl::MouseMovementCallback(float mouseXPos, float mouseYPos)
{
	m_MouseXOffset = mouseXPos - m_MouseLastX;
	m_MouseYOffset = m_MouseLastY - mouseYPos;

	m_MouseLastX = mouseXPos;
	m_MouseLastY = mouseYPos;
}

void EventSystemImpl::MouseScrollCallback(float xOffset, float yOffset)
{
}

void EventSystemImpl::ExecuteEvent(const ButtonEvent& buttonEvent)
{
	auto l_event = reinterpret_cast<std::function<void()>*>(buttonEvent.m_eventHandle);
	(*l_event)();
}

bool EventSystem::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new EventSystemImpl();
	return m_Impl->Setup();
}

bool EventSystem::Initialize()
{
	return m_Impl->Initialize();
}

bool EventSystem::Update()
{
	return m_Impl->Update();
}

bool EventSystem::Terminate()
{
	if (m_Impl->Terminate())
	{
		delete m_Impl;
		return true;
	}

	return false;
}

InputConfig EventSystem::GetInputConfig()
{
	return m_Impl->m_InputConfig;
}

void EventSystem::AddButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent)
{
	m_Impl->AddButtonStateCallback(buttonState, buttonEvent);
}

void EventSystem::AddMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent)
{
	m_Impl->AddMouseMovementCallback(mouseMovementAxis, mouseMovementEvent);
}

void EventSystem::ButtonStateCallback(ButtonState buttonState)
{
	m_Impl->ButtonStateCallback(buttonState);
}

void EventSystem::WindowResizeCallback(int32_t width, int32_t height)
{
	m_Impl->WindowResizeCallback(width, height);
}

void EventSystem::MouseMovementCallback(float mouseXPos, float mouseYPos)
{
	m_Impl->MouseMovementCallback(mouseXPos, mouseYPos);
}

void EventSystem::ScrollCallback(float xOffset, float yOffset)
{
	m_Impl->MouseScrollCallback(xOffset, yOffset);
}

Vec2 EventSystem::GetMousePosition()
{
	return m_Impl->GetMousePosition();
}

ObjectStatus EventSystem::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}