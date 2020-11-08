#pragma once
#include "../../Interface/IWindowSystem.h"
#include "MacWindowSystemBridge.h"

namespace Inno
{
	class MacWindowSystem : public IWindowSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(MacWindowSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		IWindowSurface* getWindowSurface() override;
		const std::vector<ButtonState>& getButtonState() override;

		bool sendEvent(uint32_t umsg, uint32_t WParam, int32_t LParam) override;
		bool addEventCallback(WindowEventCallbackFunctor* functor) override;

		void setBridge(MacWindowSystemBridge* bridge);
	};
}
