#pragma once
#include "IRenderingFrontend.h"

class InnoRenderingFrontend : public IRenderingFrontend
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRenderingFrontend);

	bool setup(IRenderingServer* renderingServer) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	bool runRayTrace() override;

	MeshDataComponent* addMeshDataComponent() override;
	TextureDataComponent* addTextureDataComponent() override;
	MaterialDataComponent* addMaterialDataComponent() override;
	SkeletonDataComponent* addSkeletonDataComponent() override;
	AnimationDataComponent* addAnimationDataComponent() override;

	bool registerMeshDataComponent(MeshDataComponent * rhs, bool AsyncUploadToGPU) override;
	bool registerMaterialDataComponent(MaterialDataComponent * rhs, bool AsyncUploadToGPU) override;

	MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) override;
	TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) override;
	TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;
	MaterialDataComponent* getDefaultMaterialDataComponent() override;

	bool transferDataToGPU() override;

	TVec2<uint32_t> getScreenResolution() override;
	bool setScreenResolution(TVec2<uint32_t> screenResolution) override;

	RenderingConfig getRenderingConfig() override;
	bool setRenderingConfig(RenderingConfig renderingConfig) override;

	RenderingCapability getRenderingCapability() override;

	RenderPassDesc getDefaultRenderPassDesc() override;

	const CameraGPUData& getCameraGPUData() override;
	const SunGPUData& getSunGPUData() override;
	const std::vector<CSMGPUData>& getCSMGPUData() override;
	const std::vector<PointLightGPUData>& getPointLightGPUData() override;
	const std::vector<SphereLightGPUData>& getSphereLightGPUData() override;
	const SkyGPUData& getSkyGPUData() override;

	uint32_t getSunShadowPassDrawCallCount() override;
	const std::vector<OpaquePassDrawCallData>& getSunShadowPassDrawCallData() override;
	const std::vector<MeshGPUData>& getSunShadowPassMeshGPUData() override;

	uint32_t getOpaquePassDrawCallCount() override;
	const std::vector<OpaquePassDrawCallData>& getOpaquePassDrawCallData() override;
	const std::vector<MeshGPUData>& getOpaquePassMeshGPUData() override;
	const std::vector<MaterialGPUData>& getOpaquePassMaterialGPUData() override;

	uint32_t getTransparentPassDrawCallCount() override;
	const std::vector<TransparentPassDrawCallData>& getTransparentPassDrawCallData() override;
	const std::vector<MeshGPUData>& getTransparentPassMeshGPUData() override;
	const std::vector<MaterialGPUData>& getTransparentPassMaterialGPUData() override;

	const std::vector<BillboardPassDrawCallData>& getBillboardPassDrawCallData() override;
	const std::vector<MeshGPUData>& getBillboardPassMeshGPUData() override;

	uint32_t getDebugPassDrawCallCount() override;
	const std::vector<DebugPassDrawCallData>& getDebugPassDrawCallData() override;
	const std::vector<MeshGPUData>& getDebugPassMeshGPUData() override;
};