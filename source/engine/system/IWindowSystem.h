#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"

INNO_INTERFACE IWindowSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IWindowSystem);

	virtual bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;
	virtual ButtonStatusMap getButtonStatus() = 0;

	virtual void swapBuffer() = 0;
};
