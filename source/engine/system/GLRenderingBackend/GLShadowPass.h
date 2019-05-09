#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLShadowPass
{
	void initialize();

	void update();

	GLRenderPassComponent* getGLRPC(unsigned int index);
}