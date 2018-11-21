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

	INNO_SYSTEM_EXPORT objectStatus getStatus() override;

	INNO_SYSTEM_EXPORT MeshDataComponent* getMeshDataComponent(EntityID meshID) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(EntityID textureID) override;
	INNO_SYSTEM_EXPORT MeshDataComponent* getMeshDataComponent(meshShapeType meshShapeType) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(textureType textureType) override;
	INNO_SYSTEM_EXPORT bool removeMeshDataComponent(EntityID EntityID) override;
	INNO_SYSTEM_EXPORT bool removeTextureDataComponent(EntityID EntityID) override;
	INNO_SYSTEM_EXPORT bool releaseRawDataForMeshDataComponent(EntityID EntityID) override;
	INNO_SYSTEM_EXPORT bool releaseRawDataForTextureDataComponent(EntityID EntityID) override;
	INNO_SYSTEM_EXPORT std::string loadShader(const std::string& fileName) override;
};

