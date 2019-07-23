#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/DX12RenderPassComponent.h"

namespace DX12LightPass
{
	bool initialize();
	bool update();
	bool render();
	bool terminate();
	bool resize();
	bool reloadShaders();

	DX12RenderPassComponent* getDX12RPC();
}
