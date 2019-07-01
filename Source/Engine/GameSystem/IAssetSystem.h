#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"
#include "../Component/MeshDataComponent.h"
#include "../Component/MaterialDataComponent.h"
#include "../Component/TextureDataComponent.h"
#include "../Component/VisibleComponent.h"

INNO_INTERFACE IAssetSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IAssetSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual TextureDataComponent* loadTexture(const std::string& fileName, TextureSamplerType samplerType, TextureUsageType usageType) = 0;

	virtual void addUnitCube(MeshDataComponent& meshDataComponent) = 0;
	virtual void addUnitSphere(MeshDataComponent& meshDataComponent) = 0;
	virtual void addUnitQuad(MeshDataComponent& meshDataComponent) = 0;
	virtual void addUnitLine(MeshDataComponent& meshDataComponent) = 0;
	virtual void addTerrain(MeshDataComponent& meshDataComponent) = 0;

	virtual DirectoryMetadata* getRootDirectoryMetadata() = 0;
};
