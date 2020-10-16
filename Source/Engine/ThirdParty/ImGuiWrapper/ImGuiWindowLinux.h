#pragma once
#include "IImGuiWindow.h"

class ImGuiWindowLinux : public IImGuiWindow
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWindowLinux);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool NewFrame() override;
	bool Terminate() override;
};
