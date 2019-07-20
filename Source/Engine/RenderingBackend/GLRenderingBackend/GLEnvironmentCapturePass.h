#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/InnoMath.h"
#include "../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLEnvironmentCapturePass
{
	bool initialize();
	bool update();
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();
	std::pair<std::vector<vec4>, std::vector<SH9>> getRadianceSH9();
	std::pair<std::vector<vec4>, std::vector<SH9>> getSkyVisibilitySH9();

	GLRenderPassComponent* getGLRPC();
}