#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"

#include "IRenderingFrontendSystem.h"

INNO_INTERFACE IRenderingBackendSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingBackendSystem);

	virtual bool setup(IRenderingFrontendSystem* renderingFrontend) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool resize() = 0;
	virtual bool reloadShader(RenderPassType renderPassType) = 0;
	virtual bool bakeGI() = 0;
};
