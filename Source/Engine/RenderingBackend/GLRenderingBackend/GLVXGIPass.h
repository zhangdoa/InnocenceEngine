#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLVXGIPass
{
	void initialize();

	void update();
	void draw();

	GLRenderPassComponent* getGLRPC();
}