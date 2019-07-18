#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/VKRenderPassComponent.h"

INNO_PRIVATE_SCOPE VKFinalBlendPass
{
	bool initialize();
	bool update();
	bool render();
	bool terminate();

	VKRenderPassComponent* getVKRPC();
}
