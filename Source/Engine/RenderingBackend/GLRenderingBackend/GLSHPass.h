#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/InnoMath.h"
#include "../../Component/GLRenderPassComponent.h"

namespace GLSHPass
{
	bool initialize();
	bool reloadShader();
	SH9 getSH9(GLTextureDataComponent* GLTDC);
}
