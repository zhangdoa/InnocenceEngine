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

	IResourceBinder* GetLightGrid();
	IResourceBinder* GetLightIndexList();
	IResourceBinder* GetHeatMap();
};
