#pragma once
#include "ISystem.h"
#include "../Common/MathHelper.h"

namespace Inno
{
	struct InputConfig
	{
		int32_t totalKeyCodes;
		int32_t totalMouseCodes;
	};

	enum class EventLifeTime { OneShot, Continuous };

	template<typename T>
	struct Event
	{
		EventLifeTime m_eventLifeTime = EventLifeTime::OneShot;
		T* m_eventHandle = 0;

		bool operator==(const Event& other) const
		{
			return (m_eventLifeTime == other.m_eventLifeTime
				&& m_eventHandle == other.m_eventHandle
				);
		}

		bool operator<(const Event& other) const
		{
			return m_eventHandle < other.m_eventHandle;
		}
	};

	using ButtonEvent = Event<void>;
	using MouseMovementEvent = Event<std::function<void(float)>>;
	enum class MouseMovementAxis { Horizontal, Vertical };

	class IEventSystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IEventSystem);

		virtual InputConfig getInputConfig() = 0;

		virtual void addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent) = 0;
		virtual void addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent) = 0;

		virtual void buttonStateCallback(ButtonState buttonState) = 0;
		virtual void windowSizeCallback(int32_t width, int32_t height) = 0;
		virtual void mouseMovementCallback(float mouseXPos, float mouseYPos) = 0;
		virtual void scrollCallback(float xoffset, float yoffset) = 0;

		virtual Vec2 getMousePosition() = 0;
	};
}