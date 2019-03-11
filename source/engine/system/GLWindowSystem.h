#pragma once
#include "IWindowSystem.h"

class GLWindowSystem : INNO_IMPLEMENT IWindowSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(GLWindowSystem);

	INNO_SYSTEM_EXPORT bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow) override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;
	INNO_SYSTEM_EXPORT ButtonStatusMap getButtonStatus() override;

	void swapBuffer() override;
};