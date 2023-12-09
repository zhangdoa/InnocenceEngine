#pragma once
#include "ISystem.h"
#include "IWindowSurface.h"

namespace Inno
{
	using WindowEventCallback = std::function<void(void*, uint32_t, uint64_t, int64_t)>;

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