#pragma once
#include "../../Engine/Common/GPUDataStructure.h"
#include "../../Engine/Component/GPUBufferComponent.h"

using namespace Inno;
namespace DefaultGPUBuffers
{
	enum class GPUBufferUsageType
	{
		PerFrame,
		Mesh,
		Material,
		PointLight,
		SphereLight,
		CSM,
		ComputeDispatchParam,
		GI,
		Animation,
		Billboard
	};

	bool Setup();
	bool Initialize();
	bool Upload();
	bool Terminate();

	GPUBufferComponent* GetGPUBufferComponent(GPUBufferUsageType usageType);
};