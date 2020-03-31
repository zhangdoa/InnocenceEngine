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

	bool registerMeshDataComponent(MeshDataComponent* rhs, bool AsyncUploadToGPU) override;
	bool registerMaterialDataComponent(MaterialDataComponent* rhs, bool AsyncUploadToGPU) override;
	bool registerSkeletonDataComponent(SkeletonDataComponent* rhs, bool AsyncUploadToGPU) override;
	bool registerAnimationDataComponent(AnimationDataComponent* rhs, bool AsyncUploadToGPU) override;

	MeshDataComponent* getMeshDataComponent(ProceduralMeshShape shape) override;
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

	const std::vector<DrawCallInfo>& getDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getPerObjectConstantBuffer() override;
	const std::vector<MaterialConstantBuffer>& getMaterialConstantBuffer() override;

	const std::vector<BillboardPassDrawCallInfo>& getBillboardPassDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getBillboardPassPerObjectConstantBuffer() override;

	const std::vector<DebugPassDrawCallInfo>& getDebugPassDrawCallInfo() override;
	const std::vector<PerObjectConstantBuffer>& getDebugPassPerObjectConstantBuffer() override;
};