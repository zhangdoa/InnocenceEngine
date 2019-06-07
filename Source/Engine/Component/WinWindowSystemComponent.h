#pragma once
#include "../Common/InnoType.h"
#include <SDKDDKVer.h>
#include <windows.h>
#include <windowsx.h>

class WinWindowSystemComponent
{
public:
	~WinWindowSystemComponent() {};

	static WinWindowSystemComponent& get()
	{
		static WinWindowSystemComponent instance;

		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	bool m_vsync_enabled = true;

	HINSTANCE m_hInstance;
	LPCSTR m_applicationName;
	HWND m_hwnd;
	HDC m_HDC;

private:
	WinWindowSystemComponent() {};
};
