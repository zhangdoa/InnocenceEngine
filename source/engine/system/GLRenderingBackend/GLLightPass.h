#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLLightPass
{
	void initialize();

	void update();

	bool resize();

	bool reloadShader();

	GLRenderPassComponent* getGLRPC();
}