#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLTransparentPass
{
	bool initialize();
	bool update();
	bool resize();
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}
