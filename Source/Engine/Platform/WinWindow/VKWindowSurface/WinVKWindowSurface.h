#pragma once
#include "../../../Interface/IWindowSurface.h"

class WinVKWindowSurface : public IWindowSurface
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(WinVKWindowSurface);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool Update() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	bool swapBuffer() override;
};