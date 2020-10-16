#pragma once
#include "IImGuiWindow.h"

class ImGuiWindowMac : public IImGuiWindow
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWindowMac);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool NewFrame() override;
	bool Terminate() override;
};
