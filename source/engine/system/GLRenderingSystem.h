#pragma once
#include "IRenderingBackendSystem.h"

class GLRenderingSystem : INNO_IMPLEMENT IRenderingBackendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(GLRenderingSystem);

	INNO_SYSTEM_EXPORT bool setup(IRenderingFrontendSystem* renderingFrontend) override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT bool resize() override;
};