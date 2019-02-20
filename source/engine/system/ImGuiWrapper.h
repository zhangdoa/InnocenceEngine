#pragma once
#include "../common/InnoType.h"
#include "../third-party/ImGui/imgui.h"

struct RenderingConfig
{
	bool useTAA = false;
	bool useBloom = false;
	bool useZoom = false;
	bool drawTerrain = false;
	bool drawSky = false;
	bool drawDebugObject = false;
	bool showRenderPassResult = false;
};

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

	bool addShowRenderPassResultCallback(std::function<void(RenderingConfig&)>* functor);
	bool addGetFileExplorerIconTextureIDCallback(std::function<ImTextureID(const FileExplorerIconType)>* functor);

private:
	ImGuiWrapper() {};
};