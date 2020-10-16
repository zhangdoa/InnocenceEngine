#pragma once
#include "../../Interface/IWindowSystem.h"
#include <SDKDDKVer.h>
#include <windows.h>
#include <windowsx.h>

class WinWindowSystem : public IWindowSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(WinWindowSystem);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool Update() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	IWindowSurface* getWindowSurface() override;
	const std::vector<ButtonState>& getButtonState() override;

	bool sendEvent(uint32_t umsg, uint32_t WParam, int32_t LParam) override;
	bool addEventCallback(WindowEventCallbackFunctor* functor) override;

	LPCSTR getApplicationName();
	HINSTANCE getHInstance();
	HWND getHwnd();
	bool setHwnd(HWND rhs);
};