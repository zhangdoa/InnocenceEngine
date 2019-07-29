#pragma once
#include "../../Engine/Component/GPUBufferDataComponent.h"

namespace DefaultGPUBuffers
{
	enum class GPUBufferUsageType { Camera, Mesh, Material, Sun, PointLight, SphereLight, CSM, Sky, Compute, SH9 };
	bool Initialize();
	bool Upload();

	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
};