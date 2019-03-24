#pragma once
#include "IVisionSystem.h"

class InnoVisionSystem : INNO_IMPLEMENT IVisionSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoVisionSystem);

	INNO_SYSTEM_EXPORT bool setup(void* hInstance, void* hwnd, char* pScmdline) override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT IWindowSystem* getWindowSystem() override;
	INNO_SYSTEM_EXPORT IRenderingFrontendSystem* getRenderingFrontend() override;
	INNO_SYSTEM_EXPORT IRenderingBackendSystem* getRenderingBackend() override;

	INNO_SYSTEM_EXPORT InitConfig getInitConfig() override;
};
