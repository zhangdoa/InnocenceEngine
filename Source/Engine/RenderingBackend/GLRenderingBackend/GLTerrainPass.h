#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"
#include "../../Component/GLShaderProgramComponent.h"

namespace GLTerrainPass
{
	bool initialize();
	bool update();
	bool resize(uint32_t newSizeX,  uint32_t newSizeY);
	bool reloadShader();

	GLTextureDataComponent* getHeightMap(uint32_t index);
}
