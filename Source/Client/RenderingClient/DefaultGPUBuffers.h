#pragma once
#include "../../Engine/Component/GPUBufferDataComponent.h"

namespace DefaultGPUBuffers
{
	enum class GPUBufferUsageType { MainCamera, Mesh, Material, Sun, PointLight, SphereLight, CSM, Sky, Compute, GICamera, GISky, Billboard, Debug };

	bool Setup();
	bool Initialize();
	bool Upload();
	bool Terminate();

	GPUBufferDataComponent* GetGPUBufferDataComponent(GPUBufferUsageType usageType);
};