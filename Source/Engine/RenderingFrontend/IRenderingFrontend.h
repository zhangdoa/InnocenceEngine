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
	virtual TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) = 0;
	virtual MaterialDataComponent* getDefaultMaterialDataComponent() = 0;

	virtual bool transferDataToGPU() = 0;

	virtual TVec2<uint32_t> getScreenResolution() = 0;
	virtual bool setScreenResolution(TVec2<uint32_t> screenResolution) = 0;

	virtual RenderingConfig getRenderingConfig() = 0;
	virtual bool setRenderingConfig(RenderingConfig renderingConfig) = 0;

	virtual RenderingCapability getRenderingCapability() = 0;

	virtual RenderPassDesc getDefaultRenderPassDesc() = 0;

	virtual const PerFrameConstantBuffer& getPerFrameConstantBuffer() = 0;
	virtual const std::vector<CSMConstantBuffer>& getCSMConstantBuffer() = 0;
	virtual const std::vector<PointLightConstantBuffer>& getPointLightConstantBuffer() = 0;
	virtual const std::vector<SphereLightConstantBuffer>& getSphereLightConstantBuffer() = 0;

	virtual uint32_t getSunShadowPassDrawCallCount() = 0;
	virtual const std::vector<OpaquePassDrawCallInfo>& getSunShadowPassDrawCallInfo() = 0;
	virtual const std::vector<PerObjectConstantBuffer>& getSunShadowPassPerObjectConstantBuffer() = 0;

	virtual uint32_t getOpaquePassDrawCallCount() = 0;
	virtual const std::vector<OpaquePassDrawCallInfo>& getOpaquePassDrawCallInfo() = 0;
	virtual const std::vector<PerObjectConstantBuffer>& getOpaquePassPerObjectConstantBuffer() = 0;
	virtual const std::vector<MaterialConstantBuffer>& getOpaquePassMaterialConstantBuffer() = 0;

	virtual uint32_t getTransparentPassDrawCallCount() = 0;
	virtual const std::vector<TransparentPassDrawCallInfo>& getTransparentPassDrawCallInfo() = 0;
	virtual const std::vector<PerObjectConstantBuffer>& getTransparentPassPerObjectConstantBuffer() = 0;
	virtual const std::vector<MaterialConstantBuffer>& getTransparentPassMaterialConstantBuffer() = 0;

	virtual const std::vector<BillboardPassDrawCallInfo>& getBillboardPassDrawCallInfo() = 0;
	virtual const std::vector<PerObjectConstantBuffer>& getBillboardPassPerObjectConstantBuffer() = 0;

	virtual uint32_t getDebugPassDrawCallCount() = 0;
	virtual const std::vector<DebugPassDrawCallInfo>& getDebugPassDrawCallInfo() = 0;
	virtual const std::vector<PerObjectConstantBuffer>& getDebugPassPerObjectConstantBuffer() = 0;
};