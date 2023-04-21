#pragma once
#include "IRenderingFrontend.h"

namespace Inno
{
	class RenderingFrontend : public IRenderingFrontend
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(RenderingFrontend);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool runRayTrace() override;

		MeshComponent* addMeshComponent() override;
		TextureComponent* addTextureComponent() override;
		MaterialComponent* addMaterialComponent() override;
		SkeletonComponent* addSkeletonComponent() override;
		AnimationComponent* addAnimationComponent() override;

		bool registerMeshComponent(MeshComponent* rhs, bool AsyncUploadToGPU) override;
		bool registerMaterialComponent(MaterialComponent* rhs, bool AsyncUploadToGPU) override;
		bool registerSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU) override;
		bool registerAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU) override;

		MeshComponent* getMeshComponent(ProceduralMeshShape shape) override;
		TextureComponent* getTextureComponent(WorldEditorIconType iconType) override;
		MaterialComponent* getDefaultMaterialComponent() override;

		bool transferDataToGPU() override;

		TVec2<uint32_t> getScreenResolution() override;
		bool setScreenResolution(TVec2<uint32_t> screenResolution) override;

		RenderingConfig getRenderingConfig() override;
		bool setRenderingConfig(RenderingConfig renderingConfig) override;

		RenderingCapability getRenderingCapability() override;

		RenderPassDesc getDefaultRenderPassDesc() override;

		bool playAnimation(VisibleComponent* rhs, const char* animationName, bool isLooping) override;
		bool stopAnimation(VisibleComponent* rhs, const char* animationName) override;

		const PerFrameConstantBuffer& getPerFrameConstantBuffer() override;
		const std::vector<CSMConstantBuffer>& getCSMConstantBuffer() override;
		const std::vector<PointLightConstantBuffer>& getPointLightConstantBuffer() override;
		const std::vector<SphereLightConstantBuffer>& getSphereLightConstantBuffer() override;

		const std::vector<DrawCallInfo>& getDrawCallInfo() override;
		const std::vector<PerObjectConstantBuffer>& getPerObjectConstantBuffer() override;
		const std::vector<MaterialConstantBuffer>& getMaterialConstantBuffer() override;

		const std::vector<AnimationDrawCallInfo>& getAnimationDrawCallInfo() override;
		const std::vector<AnimationConstantBuffer>& getAnimationConstantBuffer() override;

		const std::vector<BillboardPassDrawCallInfo>& getBillboardPassDrawCallInfo() override;
		const std::vector<PerObjectConstantBuffer>& getBillboardPassPerObjectConstantBuffer() override;

		const std::vector<DebugPassDrawCallInfo>& getDebugPassDrawCallInfo() override;
		const std::vector<PerObjectConstantBuffer>& getDebugPassPerObjectConstantBuffer() override;
	};
}