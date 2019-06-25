#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/GLTextureDataComponent.h"

INNO_PRIVATE_SCOPE GLBRDFLUTPass
{
	bool initialize();
	bool update();
	bool reloadShader();

	GLTextureDataComponent* getBRDFSplitSumLUT();
	GLTextureDataComponent* getBRDFMSAverageLUT();
}
