#pragma once
#include "ISystem.h"
#include "IWindowSurface.h"

namespace Inno
{
	using WindowEventCallbackFunctor = std::function<void(void*, uint32_t, uint64_t, int64_t)>;

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

		virtual IWindowSurface* getWindowSurface() = 0;
		virtual const std::vector<ButtonState>& getButtonState() = 0;

		// Editor only
		virtual bool sendEvent(uint32_t umsg, uint32_t WParam, int32_t LParam) = 0;
		virtual bool addEventCallback(WindowEventCallbackFunctor* functor) = 0;
	};
}