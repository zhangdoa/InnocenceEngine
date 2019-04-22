#pragma once
#include "../../common/InnoType.h"
#include "../../component/VKRenderPassComponent.h"

INNO_PRIVATE_SCOPE VKLightPass
{
	bool initialize();
	bool update();
	bool render();
	bool terminate();

	VKRenderPassComponent* getVKRPC();
}
