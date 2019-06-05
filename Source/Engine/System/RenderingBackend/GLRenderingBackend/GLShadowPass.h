#pragma once
#include "../../../Common/InnoType.h"
#include "../../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLShadowPass
{
	void initialize();

	void update();

	GLRenderPassComponent* getGLRPC(unsigned int index);
}