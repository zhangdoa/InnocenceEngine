#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLRenderPassComponent.h"
#include "../../Component/GLShaderProgramComponent.h"

namespace GLTerrainPass
{
	bool initialize();
	bool update();
	bool resize(unsigned int newSizeX,  unsigned int newSizeY);
	bool reloadShader();

	GLTextureDataComponent* getHeightMap(unsigned int index);
}
