#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLEnvironmentCapturePass
{
	bool initialize();
	bool update();
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
	const std::vector<GLTextureDataComponent*>& getCapturedCubemaps();
}