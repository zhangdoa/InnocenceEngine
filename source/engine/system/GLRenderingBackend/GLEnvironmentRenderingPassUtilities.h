#pragma once
#include "../../common/InnoType.h"
#include "../../component/GLTextureDataComponent.h"

INNO_PRIVATE_SCOPE GLEnvironmentRenderingPassUtilities
{
	void initialize();

	void update();

	GLTextureDataComponent* getBRDFSplitSumLUT();
	GLTextureDataComponent* getBRDFMSAverageLUT();
	GLTextureDataComponent* getConvPassGLTDC();
	GLTextureDataComponent* getPreFilterPassGLTDC();
}