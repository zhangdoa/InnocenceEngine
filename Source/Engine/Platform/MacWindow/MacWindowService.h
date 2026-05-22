#pragma once
#include "../../Common/Array.h"
#include "../../Interface/IWindowService.h"
#include "MacWindowServiceBridge.h"

namespace Inno
{
	class MacWindowService : public IWindowService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(MacWindowService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		IWindowSurface* GetWindowSurface() override;
		const Inno::Array<ButtonState>& GetButtonState() override;

		bool SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam) override;
		bool AddEventCallback(WindowEventCallback* callback) override;

		void setBridge(MacWindowServiceBridge* bridge);
	};
}
