#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLRenderPassComponent.h"
#include "../../component/GLShaderProgramComponent.h"

INNO_PRIVATE_SCOPE GLSkyPass
{
	bool initialize();
	bool update();
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
	GLShaderProgramComponent* getGLSPC();
}
