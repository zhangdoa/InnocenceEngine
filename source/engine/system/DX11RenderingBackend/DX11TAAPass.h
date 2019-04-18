#pragma once
#include "../../common/InnoType.h"
#include "../../component/DX11RenderPassComponent.h"
#include "../../component/DX11TextureDataComponent.h"

INNO_PRIVATE_SCOPE DX11TAAPass
{
	bool initialize();

	bool update();

	bool resize();

	bool reloadShaders();

	DX11RenderPassComponent* getDX11RPC();
	DX11TextureDataComponent* getResult();
}