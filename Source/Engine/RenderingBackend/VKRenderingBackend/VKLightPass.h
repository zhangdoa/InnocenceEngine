#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/VKRenderPassComponent.h"

namespace VKLightPass
{
	bool initialize();
	bool update();
	bool render();
	bool terminate();

	VKRenderPassComponent* getVKRPC();
}
