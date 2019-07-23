#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/VKRenderPassComponent.h"

namespace VKOpaquePass
{
	bool initialize();
	bool update();
	bool render();
	bool terminate();

	VKRenderPassComponent* getVKRPC();
}
