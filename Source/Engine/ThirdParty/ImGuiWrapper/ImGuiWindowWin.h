#pragma once
#include "IImGuiWindow.h"

class ImGuiWindowWin : public IImGuiWindow
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWindowWin);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool terminate() override;
};