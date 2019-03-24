#pragma once
#include "../../common/InnoType.h"
#include "../../third-party/ImGui/imgui.h"

class ImGuiWrapper
{
public:
	~ImGuiWrapper() {};

	static ImGuiWrapper& get()
	{
		static ImGuiWrapper instance;
		return instance;
	}
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool addShowRenderPassResultCallback(std::function<void()>* functor);
	bool addGetFileExplorerIconTextureIDCallback(std::function<ImTextureID(const FileExplorerIconType)>* functor);

private:
	ImGuiWrapper() {};
};