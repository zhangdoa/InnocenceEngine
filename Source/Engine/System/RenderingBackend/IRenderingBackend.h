#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/InnoClassTemplate.h"

#include "../../Component/MeshDataComponent.h"
#include "../../Component/TextureDataComponent.h"
#include "../../Component/MaterialDataComponent.h"

INNO_INTERFACE IRenderingBackend
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingBackend);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool render() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual MeshDataComponent* addMeshDataComponent() = 0;
	virtual MaterialDataComponent* addMaterialDataComponent() = 0;
	virtual TextureDataComponent* addTextureDataComponent() = 0;
	virtual MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) = 0;

	virtual void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) = 0;
	virtual void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) = 0;

	virtual bool resize() = 0;
	virtual bool reloadShader(RenderPassType renderPassType) = 0;
	virtual bool bakeGI() = 0;
};
