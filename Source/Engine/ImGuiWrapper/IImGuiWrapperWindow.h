#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

class IImGuiWrapperWindow
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IImGuiWrapperWindow);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool newFrame() = 0;
	virtual bool terminate() = 0;
};
