#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLLightPass
{
	void initialize();

	void update();

	bool resize(unsigned int newSizeX,  unsigned int newSizeY);

	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}