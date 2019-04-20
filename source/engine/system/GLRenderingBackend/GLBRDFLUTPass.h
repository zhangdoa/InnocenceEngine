#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLTextureDataComponent.h"

INNO_PRIVATE_SCOPE GLBRDFLUTPass
{
	bool initialize();
	bool update();
	bool reloadShader();

	GLTextureDataComponent* getBRDFSplitSumLUT();
	GLTextureDataComponent* getBRDFMSAverageLUT();
}
