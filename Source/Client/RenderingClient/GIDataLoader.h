#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"
#include "../../Engine/Common/GPUDataStructure.h"

namespace GIDataLoader
{
	bool Setup();
	bool Initialize();
	bool ReloadGIData();
	bool Terminate();

	const std::vector<Surfel>& GetSurfels();
	const std::vector<Brick>& GetBricks();
	const std::vector<BrickFactor>& GetBrickFactors();
	const std::vector<Probe>& GetProbes();
	const ProbeInfo& GetProbeInfo();
};
