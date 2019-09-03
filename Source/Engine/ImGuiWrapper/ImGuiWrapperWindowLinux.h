#pragma once
#include "IImGuiWrapperWindow.h"

class ImGuiWrapperWindowLinux : public IImGuiWrapperWindow
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWrapperWindowLinux);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool terminate() override;
};
