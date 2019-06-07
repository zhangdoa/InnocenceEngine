#pragma once
#include "../Core/IWindowSystem.h"
#include "MacWindowSystemBridge.h"

class MacWindowSystem : INNO_IMPLEMENT IWindowSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(MacWindowSystem);

	bool setup(void* hInstance, void* hwnd) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;
	ButtonStatusMap getButtonStatus() override;

	bool sendEvent(unsigned int umsg, unsigned int WParam, int LParam) override;

	void swapBuffer() override;

	void setBridge(MacWindowSystemBridge* bridge);
};
