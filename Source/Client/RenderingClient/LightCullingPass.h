#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

namespace LightCullingPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	bool Terminate();

	RenderPassDataComponent* GetTileFrustumRPDC();
	RenderPassDataComponent* GetLightCullingRPDC();

	IResourceBinder* GetLightGrid();
	IResourceBinder* GetLightIndexList();
	IResourceBinder* GetHeatMap();
};
