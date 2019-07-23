#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLTextureDataComponent.h"

namespace GLBRDFLUTPass
{
	bool initialize();
	bool update();
	bool reloadShader();

	GLTextureDataComponent* getBRDFSplitSumLUT();
	GLTextureDataComponent* getBRDFMSAverageLUT();
}
