#pragma once
#include "../IWindowSystem.h"
#include "../../exports/InnoSystem_Export.h"

class WinWindowSystem : INNO_IMPLEMENT IWindowSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(WinWindowSystem);

	bool setup(void* hInstance, void* hwnd) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;
	ButtonStatusMap getButtonStatus() override;

	bool sendEvent(unsigned int umsg, unsigned int WParam, int LParam) override;

	void swapBuffer() override;
};