#pragma once
#include "../common/InnoType.h"

INNO_PRIVATE_SCOPE GLRenderingSystemNS
{
	void initializeFinalPass();

	void updateFinalPass();

	bool resizeFinalPass();

	bool reloadFinalPassShaders();

	GLTextureDataComponent* updateBloomBlurPass(GLTextureDataComponent* inputGLTDC);
}