#pragma once
#include "IRenderingBackendSystem.h"

class DXRenderingSystem : INNO_IMPLEMENT IRenderingBackendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(DXRenderingSystem);

	INNO_SYSTEM_EXPORT bool setup(IRenderingFrontendSystem* renderingFrontend) override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT bool resize() override;
};
