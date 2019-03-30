#pragma once
#include "IImGuiWrapperImpl.h"

class ImGuiWrapperWinDX11 : INNO_IMPLEMENT IImGuiWrapperImpl
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(ImGuiWrapperWinDX11);

	bool setup() override;
	bool initialize() override;
	bool newFrame() override;
	bool render() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	void showRenderResult() override;
	ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType) override;
};