#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLVXGIPass
{
	void initialize();

	void update();
	void draw();

	GLRenderPassComponent* getGLRPC();
}