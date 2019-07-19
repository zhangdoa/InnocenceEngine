#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/InnoMath.h"
#include "../../Component/GLRenderPassComponent.h"

INNO_PRIVATE_SCOPE GLSHPass
{
	bool initialize();
	bool reloadShader();
	SH9 getSH9(GLTextureDataComponent* GLTDC);
}
