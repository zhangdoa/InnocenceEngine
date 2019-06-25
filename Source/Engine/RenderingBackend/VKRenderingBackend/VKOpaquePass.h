#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/VKRenderPassComponent.h"

INNO_PRIVATE_SCOPE VKOpaquePass
{
	bool initialize();
	bool update();
	bool render();
	bool terminate();

	VKRenderPassComponent* getVKRPC();
}
