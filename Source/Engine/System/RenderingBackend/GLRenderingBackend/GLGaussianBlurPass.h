#pragma once
#include "../../../Common/InnoType.h"
#include "../../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLGaussianBlurPass
{
	bool initialize();
	bool update(GLRenderPassComponent* prePassGLRPC);
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC(unsigned int index);
}
