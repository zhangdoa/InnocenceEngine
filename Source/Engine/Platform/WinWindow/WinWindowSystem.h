#pragma once
#include "../../Interface/IWindowSystem.h"
#include "../../Engine.h"
#include "../../Common/DoubleBuffer.h"
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
		bool SendEvent(void* windowHook, uint32_t uMsg, uint32_t wParam, int32_t lParam) override;
		void ConsumeEvents(const WindowEventProcessCallback& p_Callback) override;

		bool AddEventCallback(WindowEventCallback* callback) override;

		LPCSTR GetApplicationName();
		HINSTANCE GetApplicationInstance();
		HWND GetWindowHandle();
		bool SetWindowHandle(HWND hwnd);

	private:
		static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		IWindowSurface* m_WindowSurface;
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		InitConfig m_InitConfig;

		DoubleBuffer<std::vector<IWindowEvent*>> m_WindowEvents;
		std::set<WindowEventCallback*> m_WindowEventCallbacks;

		HINSTANCE m_ApplicationInstance;
		LPCSTR m_ApplicationName;
		HWND m_WindowHandle;
	};
}