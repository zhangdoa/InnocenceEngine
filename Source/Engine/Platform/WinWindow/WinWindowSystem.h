#pragma once
#include "../../Interface/IWindowSystem.h"
#include <SDKDDKVer.h>
#include <windows.h>
#include <windowsx.h>

namespace Inno
{
	class WinWindowSystem : public IWindowSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(WinWindowSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		IWindowSurface* GetWindowSurface() override;
		const std::vector<ButtonState>& GetButtonState() override;

		bool SendEvent(uint32_t uMsg, uint32_t wParam, int32_t lParam) override;
		bool AddEventCallback(WindowEventCallback* callback) override;

		LPCSTR GetApplicationName();
		HINSTANCE GetApplicationInstance();
		HWND GetWindowHandle();
		bool SetWindowHandle(HWND hwnd);
	};
}