#pragma once
#include "BaseComponent.h"

#include <windows.h>
#include <windowsx.h>

class DXWindowSystemSingletonComponent : public BaseComponent
{
public:
	~DXWindowSystemSingletonComponent() {};
	
	static DXWindowSystemSingletonComponent& getInstance()
	{
		static DXWindowSystemSingletonComponent instance;

		return instance;
	}

	HINSTANCE m_hInstance;
	PSTR m_pScmdline;
	int m_nCmdshow;
	LPCSTR m_applicationName;
	HWND m_hwnd;

private:
	DXWindowSystemSingletonComponent() {};
};
