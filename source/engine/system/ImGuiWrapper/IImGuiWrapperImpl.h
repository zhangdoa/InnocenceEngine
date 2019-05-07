#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoClassTemplate.h"
#include "../../third-party/ImGui/imgui.h"

INNO_INTERFACE IImGuiWrapperImpl
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IImGuiWrapperImpl);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool newFrame() = 0;
	virtual bool render() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual void showRenderResult(RenderPassType renderPassType) = 0;
	virtual ImTextureID getFileExplorerIconTextureID(const FileExplorerIconType iconType) = 0;
};
