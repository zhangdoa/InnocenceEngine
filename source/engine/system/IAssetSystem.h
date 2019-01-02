#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"
#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"

INNO_INTERFACE IAssetSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IAssetSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual MeshDataComponent* getMeshDataComponent(EntityID meshID) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(EntityID textureID) = 0;
	INNO_SYSTEM_EXPORT virtual MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) = 0;
	INNO_SYSTEM_EXPORT virtual bool removeMeshDataComponent(EntityID EntityID) = 0;
	INNO_SYSTEM_EXPORT virtual bool removeTextureDataComponent(EntityID EntityID) = 0;
	INNO_SYSTEM_EXPORT virtual bool releaseRawDataForMeshDataComponent(EntityID EntityID) = 0;
	INNO_SYSTEM_EXPORT virtual bool releaseRawDataForTextureDataComponent(EntityID EntityID) = 0;
	INNO_SYSTEM_EXPORT virtual std::string loadShader(const std::string& fileName) = 0;
};
