#pragma once
#include "../../../Common/InnoType.h"
#include "../../../Component/DX11RenderPassComponent.h"

INNO_PRIVATE_SCOPE DX11FinalBlendPass
{
	bool initialize();

	bool update();

	bool resize();

	bool reloadShaders();
}