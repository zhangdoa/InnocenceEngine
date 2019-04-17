#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

#include "IRenderingBackendSystem.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/MaterialDataComponent.h"

struct RenderingConfig
{
	int MSAAdepth = 4;
	bool useMotionBlur = false;
	bool useTAA = false;
	bool useBloom = false;
	bool drawTerrain = false;
	bool drawSky = false;
	bool drawDebugObject = false;
};

INNO_INTERFACE IRenderingFrontendSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingFrontendSystem);

	INNO_SYSTEM_EXPORT virtual bool setup(IRenderingBackendSystem* renderingBackendSystem) = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual MeshDataComponent* addMeshDataComponent() = 0;
	INNO_SYSTEM_EXPORT virtual MaterialDataComponent* addMaterialDataComponent() = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* addTextureDataComponent() = 0;
	INNO_SYSTEM_EXPORT virtual MeshDataComponent* getMeshDataComponent(EntityID meshID) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(EntityID textureID) = 0;
	INNO_SYSTEM_EXPORT virtual MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) = 0;
	INNO_SYSTEM_EXPORT virtual bool removeMeshDataComponent(EntityID entityID) = 0;
	INNO_SYSTEM_EXPORT virtual bool removeTextureDataComponent(EntityID entityID) = 0;

	//INNO_SYSTEM_EXPORT virtual bool registerUninitializedMeshDataComponent(MeshDataComponent* rhs) = 0;
	//INNO_SYSTEM_EXPORT virtual bool registerUninitializedTextureDataComponent(TextureDataComponent* rhs) = 0;

	INNO_SYSTEM_EXPORT virtual TVec2<unsigned int> getScreenResolution() = 0;
	INNO_SYSTEM_EXPORT virtual bool setScreenResolution(TVec2<unsigned int> screenResolution) = 0;

	INNO_SYSTEM_EXPORT virtual RenderingConfig getRenderingConfig() = 0;
	INNO_SYSTEM_EXPORT virtual bool setRenderingConfig(RenderingConfig renderingConfig) = 0;
};