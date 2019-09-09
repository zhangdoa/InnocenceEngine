#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLEnvironmentPreFilterPass
{
	bool initialize();
	bool update(GLTextureDataComponent* GLTDC);
	bool resize(uint32_t newSizeX,  uint32_t newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}