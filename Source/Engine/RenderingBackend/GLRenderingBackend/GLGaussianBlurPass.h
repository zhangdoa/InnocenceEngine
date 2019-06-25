#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLGaussianBlurPass
{
	bool initialize();
	bool update(GLRenderPassComponent* prePassGLRPC, unsigned int RTIndex, unsigned int kernel); // Kernel 0 is Gaussian9 (3 coefficients), 1 is Gaussian13 (5 coefficients), 2 is Gaussian17 (7 coefficients)
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC(unsigned int index);
}
