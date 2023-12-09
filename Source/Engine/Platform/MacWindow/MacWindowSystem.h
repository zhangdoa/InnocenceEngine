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

		IWindowSurface* GetWindowSurface() override;
		const std::vector<ButtonState>& GetButtonState() override;

		bool SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam) override;
		bool AddEventCallback(WindowEventCallback* callback) override;

		void setBridge(MacWindowSystemBridge* bridge);
	};
}
