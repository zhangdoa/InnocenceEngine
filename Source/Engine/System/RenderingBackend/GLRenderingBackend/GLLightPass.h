#pragma once
#include "../../../Common/InnoType.h"
#include "../../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLLightPass
{
	void initialize();

	void update();

	bool resize(unsigned int newSizeX,  unsigned int newSizeY);

	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}