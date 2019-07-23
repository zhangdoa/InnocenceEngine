#pragma once
#include "../../Common/InnoType.h"
#include "../../Component/DX11RenderPassComponent.h"
#include "../../Component/DX11TextureDataComponent.h"

namespace DX11TAAPass
{
	bool initialize();

	bool update();

	bool resize();

	bool reloadShaders();

	DX11RenderPassComponent* getDX11RPC();
	DX11TextureDataComponent* getResult();
}