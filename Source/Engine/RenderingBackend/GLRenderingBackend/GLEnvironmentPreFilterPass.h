#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLEnvironmentPreFilterPass
{
	bool initialize();
	bool update(GLTextureDataComponent* GLTDC);
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}