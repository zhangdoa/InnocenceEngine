#pragma once
#include "../common/InnoType.h"
#include "../common/InnoClassTemplate.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/MaterialDataComponent.h"

INNO_INTERFACE IRenderingBackendSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingBackendSystem);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool render() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual MeshDataComponent* addMeshDataComponent() = 0;
	virtual MaterialDataComponent* addMaterialDataComponent() = 0;
	virtual TextureDataComponent* addTextureDataComponent() = 0;
	virtual MeshDataComponent* getMeshDataComponent(EntityID meshID) = 0;
	virtual TextureDataComponent* getTextureDataComponent(EntityID textureID) = 0;
	virtual MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) = 0;
	virtual bool removeMeshDataComponent(EntityID entityID) = 0;
	virtual bool removeTextureDataComponent(EntityID entityID) = 0;

	virtual void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) = 0;
	virtual void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) = 0;

	virtual bool resize() = 0;
	virtual bool reloadShader(RenderPassType renderPassType) = 0;
	virtual bool bakeGI() = 0;
};
