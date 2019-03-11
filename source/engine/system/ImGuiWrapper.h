#pragma once
#include "../common/InnoType.h"
#include "../third-party/ImGui/imgui.h"

class ImGuiWrapper
{
public:
	~ImGuiWrapper() {};

	static ImGuiWrapper& get()
	{
		static ImGuiWrapper instance;
		return instance;
	}
	void setup();
	void initialize();
	void update();
	void terminate();

	bool addShowRenderPassResultCallback(std::function<void()>* functor);
	bool addGetFileExplorerIconTextureIDCallback(std::function<ImTextureID(const FileExplorerIconType)>* functor);

private:
	ImGuiWrapper() {};
};