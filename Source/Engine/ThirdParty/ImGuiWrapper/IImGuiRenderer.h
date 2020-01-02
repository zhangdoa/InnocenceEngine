#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/InnoClassTemplate.h"
#include "../ImGui/imgui.h"

enum class RenderPassType { Shadow, GI, Opaque, Light, Transparent, Terrain, PostProcessing, Development };

class IImGuiRenderer
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IImGuiRenderer);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool newFrame() = 0;
	virtual bool render() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual void showRenderResult(RenderPassType renderPassType) = 0;
};
