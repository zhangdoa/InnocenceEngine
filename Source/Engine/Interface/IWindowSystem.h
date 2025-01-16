#pragma once
#include "../Common/STL14.h"
#include "ISystem.h"
#include "IWindowSurface.h"

namespace Inno
{
	using WindowEventCallback = std::function<void(void*, uint32_t, uint64_t, int64_t)>;

	struct ButtonState
	{
		int32_t m_code = 0;
		bool m_isPressed = false;

		bool operator==(const ButtonState& other) const
		{
			return (
				m_code == other.m_code
				&& m_isPressed == other.m_isPressed
				);
		}
	};

	struct ButtonStateHasher
	{
		std::size_t operator()(const ButtonState& k) const
		{
			return std::hash<int32_t>()(k.m_code) ^ (std::hash<bool>()(k.m_isPressed) << 1);
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
		virtual const std::vector<ButtonState>& GetButtonState() = 0;

		// Editor only
		virtual bool SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam) = 0;
		virtual bool AddEventCallback(WindowEventCallback* callback) = 0;
	};
}