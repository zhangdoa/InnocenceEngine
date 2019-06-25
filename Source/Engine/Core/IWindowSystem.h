#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

INNO_INTERFACE IWindowSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IWindowSystem);

	virtual bool setup(void* hInstance, void* hwnd) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;
	virtual ButtonStatusMap getButtonStatus() = 0;

	// Editor only
	virtual bool sendEvent(unsigned int umsg, unsigned int WParam, int LParam) = 0;

	virtual void swapBuffer() = 0;
};
