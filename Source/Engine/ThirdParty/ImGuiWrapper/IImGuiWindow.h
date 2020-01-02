#pragma once
#include "../../Common/InnoClassTemplate.h"

class IImGuiWindow
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IImGuiWindow);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool newFrame() = 0;
	virtual bool terminate() = 0;
};
