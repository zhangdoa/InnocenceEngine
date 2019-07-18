#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

INNO_INTERFACE IWindowSurface
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IWindowSurface);

	virtual bool setup(void* hInstance, void* hwnd, void* WindowProc) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool swapBuffer() = 0;
};
