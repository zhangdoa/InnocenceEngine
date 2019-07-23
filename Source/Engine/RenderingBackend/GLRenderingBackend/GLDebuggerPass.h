#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLDebuggerPass
{
	bool initialize();
	bool update(GLRenderPassComponent* canvas);
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC(unsigned int index);
}
