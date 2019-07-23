#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLTransparentPass
{
	bool initialize();
	bool update(GLRenderPassComponent* prePassGLRPC);
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();
}
