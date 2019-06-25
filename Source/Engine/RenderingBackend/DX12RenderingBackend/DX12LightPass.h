#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/DX12RenderPassComponent.h"

INNO_PRIVATE_SCOPE DX12LightPass
{
	bool initialize();
	bool update();
	bool render();
	bool resize();
	bool reloadShaders();

	DX12RenderPassComponent* getDX12RPC();
}
