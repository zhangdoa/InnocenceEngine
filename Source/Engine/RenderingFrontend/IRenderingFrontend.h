#pragma once
#include "../Common/InnoType.h"

#include "../Common/InnoClassTemplate.h"

#include "../RenderingServer/IRenderingServer.h"

#include "../Component/MeshDataComponent.h"
#include "../Component/TextureDataComponent.h"
#include "../Component/MaterialDataComponent.h"
#include "../Component/SkeletonDataComponent.h"
#include "../Component/AnimationDataComponent.h"

#include "../Common/GPUDataStructure.h"

struct RenderingConfig
{
	bool VSync = false;
	int32_t MSAAdepth = 4;
	bool useMotionBlur = false;
	bool useTAA = false;
	bool useBloom = false;
	bool drawTerrain = false;
	bool drawSky = false;
	bool drawDebugObject = false;
	bool CSMFitToScene = false;
	bool CSMAdjustDrawDistance = false;
	bool CSMAdjustSidePlane = false;
};

struct RenderingCapability
{
	uint32_t maxCSMSplits;
	uint32_t maxPointLights;
	uint32_t maxSphereLights;
	uint32_t maxMeshes;
	uint32_t maxMaterials;
	uint32_t maxTextures;
};

enum class WorldEditorIconType { DIRECTIONAL_LIGHT, POINT_LIGHT, SPHERE_LIGHT, UNKNOWN };

class IRenderingFrontend
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingFrontend);

	virtual bool setup(IRenderingServer* renderingServer) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool runRayTrace() = 0;

	virtual MeshDataComponent* addMeshDataComponent() = 0;
	virtual TextureDataComponent* addTextureDataComponent() = 0;
	virtual MaterialDataComponent* addMaterialDataComponent() = 0;
	virtual SkeletonDataComponent* addSkeletonDataComponent() = 0;
	virtual AnimationDataComponent* addAnimationDataComponent() = 0;

	virtual bool registerMeshDataComponent(MeshDataComponent * rhs, bool AsyncUploadToGPU = true) = 0;
	virtual bool registerMaterialDataComponent(MaterialDataComponent * rhs, bool AsyncUploadToGPU = true) = 0;

	virtual MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(TextureAttributeType textureAttributeType) = 0;
	virtual TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) = 0;
	virtual MaterialDataComponent* getDefaultMaterialDataComponent() = 0;

	virtual bool transferDataToGPU() = 0;

	virtual TVec2<uint32_t> getScreenResolution() = 0;
	virtual bool setScreenResolution(TVec2<uint32_t> screenResolution) = 0;

	virtual RenderingConfig getRenderingConfig() = 0;
	virtual bool setRenderingConfig(RenderingConfig renderingConfig) = 0;

	virtual RenderingCapability getRenderingCapability() = 0;

	virtual RenderPassDesc getDefaultRenderPassDesc() = 0;

	virtual const CameraGPUData& getCameraGPUData() = 0;
	virtual const SunGPUData& getSunGPUData() = 0;
	virtual const std::vector<CSMGPUData>& getCSMGPUData() = 0;
	virtual const std::vector<PointLightGPUData>& getPointLightGPUData() = 0;
	virtual const std::vector<SphereLightGPUData>& getSphereLightGPUData() = 0;
	virtual const SkyGPUData& getSkyGPUData() = 0;

	virtual uint32_t getSunShadowPassDrawCallCount() = 0;
	virtual const std::vector<OpaquePassDrawCallData>& getSunShadowPassDrawCallData() = 0;
	virtual const std::vector<MeshGPUData>& getSunShadowPassMeshGPUData() = 0;

	virtual uint32_t getOpaquePassDrawCallCount() = 0;
	virtual const std::vector<OpaquePassDrawCallData>& getOpaquePassDrawCallData() = 0;
	virtual const std::vector<MeshGPUData>& getOpaquePassMeshGPUData() = 0;
	virtual const std::vector<MaterialGPUData>& getOpaquePassMaterialGPUData() = 0;

	virtual uint32_t getTransparentPassDrawCallCount() = 0;
	virtual const std::vector<TransparentPassDrawCallData>& getTransparentPassDrawCallData() = 0;
	virtual const std::vector<MeshGPUData>& getTransparentPassMeshGPUData() = 0;
	virtual const std::vector<MaterialGPUData>& getTransparentPassMaterialGPUData() = 0;

	virtual const std::vector<BillboardPassDrawCallData>& getBillboardPassDrawCallData() = 0;
	virtual const std::vector<MeshGPUData>& getBillboardPassMeshGPUData() = 0;

	virtual uint32_t getDebugPassDrawCallCount() = 0;
	virtual const std::vector<DebugPassDrawCallData>& getDebugPassDrawCallData() = 0;
	virtual const std::vector<MeshGPUData>& getDebugPassMeshGPUData() = 0;
};