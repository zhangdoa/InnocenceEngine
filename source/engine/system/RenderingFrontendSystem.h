#pragma once
#include "IRenderingFrontendSystem.h"

INNO_CONCRETE InnoRenderingFrontendSystem : INNO_IMPLEMENT IRenderingFrontendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRenderingFrontendSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT void loadDefaultAssets() override;

	INNO_SYSTEM_EXPORT MeshDataComponent* addMeshDataComponent() override;
	INNO_SYSTEM_EXPORT MaterialDataComponent* addMaterialDataComponent() override;
	INNO_SYSTEM_EXPORT TextureDataComponent* addTextureDataComponent() override;
	INNO_SYSTEM_EXPORT MeshDataComponent* getMeshDataComponent(EntityID meshID) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(EntityID textureID) override;
	INNO_SYSTEM_EXPORT MeshDataComponent* getMeshDataComponent(MeshShapeType MeshShapeType) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(TextureUsageType TextureUsageType) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;
	INNO_SYSTEM_EXPORT bool removeMeshDataComponent(EntityID EntityID) override;
	INNO_SYSTEM_EXPORT bool removeTextureDataComponent(EntityID EntityID) override;
	INNO_SYSTEM_EXPORT bool releaseRawDataForMeshDataComponent(EntityID EntityID) override;
	INNO_SYSTEM_EXPORT bool releaseRawDataForTextureDataComponent(EntityID EntityID) override;

	INNO_SYSTEM_EXPORT void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) override;
	INNO_SYSTEM_EXPORT void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) override;

	INNO_SYSTEM_EXPORT TVec2<unsigned int> getScreenResolution() override;
	INNO_SYSTEM_EXPORT bool setScreenResolution(TVec2<unsigned int> screenResolution) override;

	INNO_SYSTEM_EXPORT RenderingConfig getRenderingConfig() override;
	INNO_SYSTEM_EXPORT bool setRenderingConfig(RenderingConfig renderingConfig) override;
};