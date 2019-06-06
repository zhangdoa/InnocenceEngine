#pragma once
#include "../../Common/InnoType.h"

#include "../../Common/InnoClassTemplate.h"

#include "../RenderingBackend/IRenderingBackend.h"

#include "../../Component/MeshDataComponent.h"
#include "../../Component/TextureDataComponent.h"
#include "../../Component/MaterialDataComponent.h"
#include "../../Component/SkeletonDataComponent.h"
#include "../../Component/AnimationDataComponent.h"

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

INNO_INTERFACE IRenderingFrontend
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingFrontend);

	virtual bool setup(IRenderingBackend* renderingBackend) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual MeshDataComponent* addMeshDataComponent() = 0;
	virtual MaterialDataComponent* addMaterialDataComponent() = 0;
	virtual TextureDataComponent* addTextureDataComponent() = 0;
	virtual MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) = 0;
	virtual SkeletonDataComponent* addSkeletonDataComponent() = 0;
	virtual AnimationDataComponent* addAnimationDataComponent() = 0;

	virtual TVec2<unsigned int> getScreenResolution() = 0;
	virtual bool setScreenResolution(TVec2<unsigned int> screenResolution) = 0;

	virtual RenderingConfig getRenderingConfig() = 0;
	virtual bool setRenderingConfig(RenderingConfig renderingConfig) = 0;
};