#pragma once
#include "IImGuiWindow.h"

class ImGuiWindowLinux : public IImGuiWindow
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWindowLinux);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool terminate() override;
};
