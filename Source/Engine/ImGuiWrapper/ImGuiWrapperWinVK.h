#pragma once
#include "IImGuiWrapperImpl.h"

class ImGuiWrapperWinVK : public IImGuiWrapperImpl
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWrapperWinVK);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool render() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void showRenderResult(RenderPassType renderPassType) override;
	ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType) override;
};