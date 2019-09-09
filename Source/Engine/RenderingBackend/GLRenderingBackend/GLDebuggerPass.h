#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLDebuggerPass
{
	bool initialize();
	bool update(GLRenderPassComponent* canvas);
	bool resize(uint32_t newSizeX,  uint32_t newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC(uint32_t index);
}
