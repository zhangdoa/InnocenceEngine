#pragma once
#include "IVisionSystem.h"

class InnoVisionSystem : INNO_IMPLEMENT IVisionSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoVisionSystem);

	INNO_SYSTEM_EXPORT bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow) override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;

	INNO_SYSTEM_EXPORT bool resize() override;
};
