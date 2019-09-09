#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLPreTAAPass
{
	bool initialize();
	bool update();
	bool resize(uint32_t newSizeX,  uint32_t newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}
