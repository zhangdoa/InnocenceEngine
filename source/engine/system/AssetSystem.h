#pragma once
#include "IAssetSystem.h"

class InnoAssetSystem : INNO_IMPLEMENT IAssetSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoAssetSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT TextureDataComponent* loadTexture(const std::string& fileName, TextureSamplerType samplerType, TextureUsageType usageType) override;

	INNO_SYSTEM_EXPORT void loadAssetsForComponents() override;

	INNO_SYSTEM_EXPORT void addUnitCube(MeshDataComponent& meshDataComponent) override;
	INNO_SYSTEM_EXPORT void addUnitSphere(MeshDataComponent& meshDataComponent) override;
	INNO_SYSTEM_EXPORT void addUnitQuad(MeshDataComponent& meshDataComponent) override;
	INNO_SYSTEM_EXPORT void addUnitLine(MeshDataComponent& meshDataComponent) override;
	INNO_SYSTEM_EXPORT void addTerrain(MeshDataComponent& meshDataComponent) override;

	INNO_SYSTEM_EXPORT DirectoryMetadata* getRootDirectoryMetadata() override;
};
