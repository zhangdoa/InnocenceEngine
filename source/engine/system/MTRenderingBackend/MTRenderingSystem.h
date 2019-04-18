#pragma once
#include "../IRenderingBackendSystem.h"
#include "MTRenderingSystemBridge.h"

class MTRenderingSystem : INNO_IMPLEMENT IRenderingBackendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(MTRenderingSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool render() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	MeshDataComponent* addMeshDataComponent() override;
	MaterialDataComponent* addMaterialDataComponent() override;
	TextureDataComponent* addTextureDataComponent() override;
	MeshDataComponent* getMeshDataComponent(EntityID meshID) override;
	TextureDataComponent* getTextureDataComponent(EntityID textureID) override;
	MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) override;
	TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) override;
	TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) override;
	TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;
	bool removeMeshDataComponent(EntityID entityID) override;
	bool removeTextureDataComponent(EntityID entityID) override;

	void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) override;
	void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) override;

	bool resize() override;
	bool reloadShader(RenderPassType renderPassType) override;
	bool bakeGI() override;

	void MTRenderingSystem::setBridge(MTRenderingSystemBridge* bridge);
};
