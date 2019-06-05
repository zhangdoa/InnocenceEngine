#pragma once
#include "../../../Common/InnoType.h"
#include "../../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLVXGIPass
{
	void initialize();

	void update();
	void draw();

	GLRenderPassComponent* getGLRPC();
}