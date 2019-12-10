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
	TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;
	MaterialDataComponent* getDefaultMaterialDataComponent() override;

	bool transferDataToGPU() override;

	TVec2<uint32_t> getScreenResolution() override;
	bool setScreenResolution(TVec2<uint32_t> screenResolution) override;

	RenderingConfig getRenderingConfig() override;
	bool setRenderingConfig(RenderingConfig renderingConfig) override;

	RenderingCapability getRenderingCapability() override;

	RenderPassDesc getDefaultRenderPassDesc() override;

	const PerFrameConstantBuffer& getPerFrameConstantBuffer() override;
	const std::vector<CSMConstantBuffer>& getCSMConstantBuffer() override;
	const std::vector<PointLightConstantBuffer>& getPointLightConstantBuffer() override;
	const std::vector<SphereLightConstantBuffer>& getSphereLightConstantBuffer() override;

	uint32_t getSunShadowPassDrawCallCount() override;
	const std::vector<OpaquePassDrawCallInfo>& getSunShadowPassDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getSunShadowPassPerObjectConstantBuffer() override;

	uint32_t getOpaquePassDrawCallCount() override;
	const std::vector<OpaquePassDrawCallInfo>& getOpaquePassDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getOpaquePassPerObjectConstantBuffer() override;
	const std::vector<MaterialConstantBuffer>& getOpaquePassMaterialConstantBuffer() override;

	uint32_t getTransparentPassDrawCallCount() override;
	const std::vector<TransparentPassDrawCallInfo>& getTransparentPassDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getTransparentPassPerObjectConstantBuffer() override;
	const std::vector<MaterialConstantBuffer>& getTransparentPassMaterialConstantBuffer() override;

	const std::vector<BillboardPassDrawCallInfo>& getBillboardPassDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getBillboardPassPerObjectConstantBuffer() override;

	uint32_t getDebugPassDrawCallCount() override;
	const std::vector<DebugPassDrawCallInfo>& getDebugPassDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getDebugPassPerObjectConstantBuffer() override;
};