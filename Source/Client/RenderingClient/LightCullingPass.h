#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

using namespace Inno;
namespace LightCullingPass
{
	bool Setup();
	bool Initialize();
	bool PrepareCommandList();

	bool Terminate();

	RenderPassDataComponent* GetTileFrustumRPDC();
	RenderPassDataComponent* GetLightCullingRPDC();

	GPUResourceComponent* GetLightGrid();
	GPUResourceComponent* GetLightIndexList();
	GPUResourceComponent* GetHeatMap();
};
