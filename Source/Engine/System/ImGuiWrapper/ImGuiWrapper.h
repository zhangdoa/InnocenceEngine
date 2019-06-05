#pragma once
#include "../../Common/InnoType.h"
#include "../../ThirdParty/ImGui/imgui.h"

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

private:
	ImGuiWrapper() {};
};