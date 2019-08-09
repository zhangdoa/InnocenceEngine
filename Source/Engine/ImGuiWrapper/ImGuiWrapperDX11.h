#pragma once
#include "IImGuiWrapperRenderer.h"

class ImGuiWrapperDX11 : public IImGuiWrapperRenderer
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWrapperDX11);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool render() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void showRenderResult(RenderPassType renderPassType) override;
	ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType) override;
};