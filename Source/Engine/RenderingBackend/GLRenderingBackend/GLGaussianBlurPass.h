#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLGaussianBlurPass
{
	bool initialize();
	bool update(GLRenderPassComponent* prePassGLRPC, uint32_t RTIndex, uint32_t kernel); // Kernel 0 is Gaussian9 (3 coefficients), 1 is Gaussian13 (5 coefficients), 2 is Gaussian17 (7 coefficients)
	bool resize(uint32_t newSizeX,  uint32_t newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC(uint32_t index);
}
