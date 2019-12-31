#pragma once
#include "IImGuiWrapperWindow.h"

class ImGuiWrapperWindowMac : public IImGuiWrapperWindow
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWrapperWindowMac);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool terminate() override;
};
