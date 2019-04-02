#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLTAAPass
{
	bool initialize();
	bool update(GLRenderPassComponent* prePassGLRPC);
	bool resize();
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}
