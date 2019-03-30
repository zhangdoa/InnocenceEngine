#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

#include "IWindowSystem.h"
#include "IRenderingFrontendSystem.h"
#include "IRenderingBackendSystem.h"

enum EngineMode { GAME, EDITOR };
enum RenderingBackend { GL, DX11, DX12, VK };
struct InitConfig
{
	EngineMode engineMode = EngineMode::GAME;
	RenderingBackend renderingBackend = RenderingBackend::GL;
};

INNO_INTERFACE IVisionSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IVisionSystem);

	INNO_SYSTEM_EXPORT virtual bool setup(void* hInstance, void* hwnd, char* pScmdline) = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual IWindowSystem* getWindowSystem() = 0;
	INNO_SYSTEM_EXPORT virtual IRenderingFrontendSystem* getRenderingFrontend() = 0;
	INNO_SYSTEM_EXPORT virtual IRenderingBackendSystem* getRenderingBackend() = 0;

	INNO_SYSTEM_EXPORT virtual InitConfig getInitConfig() = 0;
};
