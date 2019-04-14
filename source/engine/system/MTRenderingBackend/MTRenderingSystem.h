#pragma once
#include "../IRenderingBackendSystem.h"
#include "MTRenderingSystemBridge.h"

class MTRenderingSystem : INNO_IMPLEMENT IRenderingBackendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(MTRenderingSystem);

	bool setup(IRenderingFrontendSystem* renderingFrontend) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	bool resize() override;
	bool reloadShader(RenderPassType renderPassType) override;
	bool bakeGI() override;

	void setBridge(MTRenderingSystemBridge* bridge);
};
