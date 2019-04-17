#pragma once
#include "IRenderingFrontendSystem.h"

INNO_CONCRETE InnoRenderingFrontendSystem : INNO_IMPLEMENT IRenderingFrontendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRenderingFrontendSystem);

	INNO_SYSTEM_EXPORT bool setup(IRenderingBackendSystem* renderingBackendSystem) override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT MeshDataComponent* addMeshDataComponent() override;
	INNO_SYSTEM_EXPORT MaterialDataComponent* addMaterialDataComponent() override;
	INNO_SYSTEM_EXPORT TextureDataComponent* addTextureDataComponent() override;
	INNO_SYSTEM_EXPORT MeshDataComponent* getMeshDataComponent(EntityID meshID) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(EntityID textureID) override;
	INNO_SYSTEM_EXPORT MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) override;
	INNO_SYSTEM_EXPORT TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;
	INNO_SYSTEM_EXPORT bool removeMeshDataComponent(EntityID entityID) override;
	INNO_SYSTEM_EXPORT bool removeTextureDataComponent(EntityID entityID) override;

	//INNO_SYSTEM_EXPORT bool registerUninitializedMeshDataComponent(MeshDataComponent* rhs) override;
	//INNO_SYSTEM_EXPORT bool registerUninitializedTextureDataComponent(TextureDataComponent* rhs) override;

	INNO_SYSTEM_EXPORT TVec2<unsigned int> getScreenResolution() override;
	INNO_SYSTEM_EXPORT bool setScreenResolution(TVec2<unsigned int> screenResolution) override;

	INNO_SYSTEM_EXPORT RenderingConfig getRenderingConfig() override;
	INNO_SYSTEM_EXPORT bool setRenderingConfig(RenderingConfig renderingConfig) override;
};