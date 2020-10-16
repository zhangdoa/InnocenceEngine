#pragma once
#include "../../Engine/Interface/ISystem.h"
#include "../ImGui/imgui.h"

enum class RenderPassType { Shadow, GI, Opaque, Light, Transparent, Terrain, PostProcessing, Development };

class IImGuiRenderer : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IImGuiRenderer);

	virtual bool NewFrame() = 0;
	virtual bool Render() = 0;

	virtual void ShowRenderResult(RenderPassType renderPassType) = 0;
};
