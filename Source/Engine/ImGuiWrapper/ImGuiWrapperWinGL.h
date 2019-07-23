#pragma once
#include "IImGuiWrapperImpl.h"

class ImGuiWrapperWinGL : public IImGuiWrapperImpl
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWrapperWinGL);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool render() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void showRenderResult(RenderPassType renderPassType) override;
	ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType) override;
};