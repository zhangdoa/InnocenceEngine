#pragma once
#include "../../../Core/IWindowSurface.h"

class WinVKWindowSurface : public IWindowSurface
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(WinVKWindowSurface);

	bool setup(void* hInstance, void* hwnd, void* WindowProc) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	bool swapBuffer() override;
};