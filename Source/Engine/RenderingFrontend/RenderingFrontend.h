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

	bool registerMeshDataComponent(MeshDataComponent * rhs) override;
	bool registerMaterialDataComponent(MaterialDataComponent * rhs) override;

	MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) override;
	TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) override;
	TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) override;
	TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;
	MaterialDataComponent* getDefaultMaterialDataComponent() override;

	bool transferDataToGPU() override;

	TVec2<unsigned int> getScreenResolution() override;
	bool setScreenResolution(TVec2<unsigned int> screenResolution) override;

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

	unsigned int getOpaquePassDrawCallCount() override;
	const std::vector<OpaquePassDrawCallData>& getOpaquePassDrawCallData() override;
	const std::vector<MeshGPUData>& getOpaquePassMeshGPUData() override;
	const std::vector<MaterialGPUData>& getOpaquePassMaterialGPUData() override;

	unsigned int getTransparentPassDrawCallCount() override;
	const std::vector<TransparentPassDrawCallData>& getTransparentPassDrawCallData() override;
	const std::vector<MeshGPUData>& getTransparentPassMeshGPUData() override;
	const std::vector<MaterialGPUData>& getTransparentPassMaterialGPUData() override;

	const std::vector<BillboardPassDrawCallData>& getBillboardPassDrawCallData() override;
	const std::vector<MeshGPUData>& getBillboardPassMeshGPUData() override;

	unsigned int getDebuggerPassDrawCallCount() override;
	const std::vector<DebuggerPassGPUData>& getDebuggerPassGPUData() override;

	unsigned int getGIPassDrawCallCount() override;
	const std::vector<OpaquePassDrawCallData>& getGIPassGPUData() override;
	const std::vector<MeshGPUData>& getGIPassMeshGPUData() override;
	const std::vector<MaterialGPUData>& getGIPassMaterialGPUData() override;
};