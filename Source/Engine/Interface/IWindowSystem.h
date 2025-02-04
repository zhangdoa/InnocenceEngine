#pragma once
#include "../Common/STL14.h"
#include "../Common/Enum.h"
#include "ISystem.h"
#include "IWindowSurface.h"

INNO_ENUM(WindowEventType, Button, Mouse);

namespace Inno
{
	using WindowEventCallback = std::function<void(void*, uint32_t, uint64_t, int64_t)>;

	struct IWindowEvent
	{
		virtual ~IWindowEvent() = default;
		virtual WindowEventType GetType() const = 0;
	};

	using WindowEventProcessCallback = std::function<void(const std::vector<IWindowEvent*>&)>;

	struct ButtonState : public IWindowEvent
	{
		ButtonState(int32_t p_Code, bool p_IsPressed)
			: m_Code(p_Code)
			, m_isPressed(p_IsPressed)
		{
		}

		bool operator==(const ButtonState& other) const
		{
			return (
				m_Code == other.m_Code
				&& m_isPressed == other.m_isPressed
				);
		}

		WindowEventType GetType() const override
		{
			return WindowEventType::Button;
		}

		int32_t m_Code = 0;
		bool m_isPressed = false;
	};

	struct MouseState : public IWindowEvent
	{
		MouseState(int32_t p_X, int32_t p_Y)
			: m_X(p_X)
			, m_Y(p_Y)
		{
		}

		WindowEventType GetType() const override
		{
			return WindowEventType::Mouse;
		}

		int32_t m_X = 0;
		int32_t m_Y = 0;
	};

	struct ButtonStateHasher
	{
		std::size_t operator()(const ButtonState& k) const
		{
			return std::hash<int32_t>()(k.m_Code) ^ (std::hash<bool>()(k.m_isPressed) << 1);
		}
	};

	class IWindowSystemConfig : public ISystemConfig
	{
	public:
		void* m_AppHook;
		void* m_ExtraHook;
	};

	class IWindowSystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IWindowSystem);

		virtual IWindowSurface* GetWindowSurface() = 0;
		virtual bool SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam) = 0;
		virtual void ConsumeEvents(const WindowEventProcessCallback& p_Callback) = 0;

		// Editor only
		virtual bool AddEventCallback(WindowEventCallback* callback) = 0;
	};
}