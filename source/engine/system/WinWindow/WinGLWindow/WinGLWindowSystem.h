#pragma once
#include "../IWinWindowSystem.h"
#include "../../../exports/InnoSystem_Export.h"

class WinGLWindowSystem : INNO_IMPLEMENT IWinWindowSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(WinGLWindowSystem);

	bool setup(void* hInstance, void* hwnd, void* WindowProc) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void swapBuffer() override;
};