#pragma once
#include "../../common/InnoType.h"
#include "../../component/VKRenderPassComponent.h"

INNO_PRIVATE_SCOPE VKOpaquePass
{
	bool initialize();
	bool update();
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	VKRenderPassComponent* getVKRPC();
}
