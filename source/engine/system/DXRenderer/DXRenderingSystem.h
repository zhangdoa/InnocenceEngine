#pragma once
#include "../../common/InnoType.h"
#include "../../component/AssetSystemSingletonComponent.h"

namespace DXRenderingSystem
{
	void setup();
	void initialize();
	void update();
	void shutdown();

	void initializeDefaultGraphicPrimtives();
	void initializeGraphicPrimtivesOfComponents();
	void initializeMesh(MeshDataComponent* DXMeshDataComponent);

	objectStatus getStatus();
};
