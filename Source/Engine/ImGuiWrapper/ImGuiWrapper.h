#pragma once
#include "../Common/InnoType.h"

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
	bool render();
	bool terminate();

private:
	ImGuiWrapper() {};
};