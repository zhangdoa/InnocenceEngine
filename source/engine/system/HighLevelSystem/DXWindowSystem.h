#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "IWindowSystem.h"

class DXWindowSystem : IMPLEMENT IWindowSystem
{
public:
	InnoHighLevelSystem_EXPORT bool setup(void* hInstance, void* hPrevInstance, char* pScmdline, int nCmdshow) override;
	InnoHighLevelSystem_EXPORT bool initialize() override;
	InnoHighLevelSystem_EXPORT bool update() override;
	InnoHighLevelSystem_EXPORT bool terminate() override;

	InnoHighLevelSystem_EXPORT objectStatus getStatus() override;

	void swapBuffer() override;
};