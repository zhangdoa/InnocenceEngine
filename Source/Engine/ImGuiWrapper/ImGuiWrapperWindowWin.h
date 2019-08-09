#pragma once
#include "IImGuiWrapperWindow.h"

class ImGuiWrapperWindowWin : public IImGuiWrapperWindow
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWrapperWindowWin);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool terminate() override;
};