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

		bool RunRayTracing() override;

		MeshComponent* AddMeshComponent() override;
		TextureComponent* AddTextureComponent() override;
		MaterialComponent* AddMaterialComponent() override;
		SkeletonComponent* AddSkeletonComponent() override;
		AnimationComponent* AddAnimationComponent() override;

		bool InitializeMeshComponent(MeshComponent* rhs, bool AsyncUploadToGPU) override;
		bool InitializeMaterialComponent(MaterialComponent* rhs, bool AsyncUploadToGPU) override;
		bool InitializeSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU) override;
		bool InitializeAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU) override;

		MeshComponent* GetMeshComponent(ProceduralMeshShape shape) override;
		TextureComponent* GetTextureComponent(WorldEditorIconType iconType) override;
		MaterialComponent* GetDefaultMaterialComponent() override;

		bool TransferDataToGPU() override;

		TVec2<uint32_t> GetScreenResolution() override;
		bool SetScreenResolution(TVec2<uint32_t> screenResolution) override;

		RenderingConfig GetRenderingConfig() override;
		bool SetRenderingConfig(RenderingConfig renderingConfig) override;

		RenderingCapability GetRenderingCapability() override;

		RenderPassDesc GetDefaultRenderPassDesc() override;

		bool PlayAnimation(VisibleComponent* rhs, const char* animationName, bool isLooping) override;
		bool StopAnimation(VisibleComponent* rhs, const char* animationName) override;

		const PerFrameConstantBuffer& GetPerFrameConstantBuffer() override;
		const std::vector<CSMConstantBuffer>& GetCSMConstantBuffer() override;
		const std::vector<PointLightConstantBuffer>& GetPointLightConstantBuffer() override;
		const std::vector<SphereLightConstantBuffer>& GetSphereLightConstantBuffer() override;

		const std::vector<DrawCallInfo>& GetDrawCallInfo() override;
		const std::vector<PerObjectConstantBuffer>& GetPerObjectConstantBuffer() override;
		const std::vector<MaterialConstantBuffer>& GetMaterialConstantBuffer() override;

		const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo() override;
		const std::vector<AnimationConstantBuffer>& GetAnimationConstantBuffer() override;

		const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo() override;
		const std::vector<PerObjectConstantBuffer>& GetBillboardPassPerObjectConstantBuffer() override;

		const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo() override;
		const std::vector<PerObjectConstantBuffer>& GetDebugPassPerObjectConstantBuffer() override;
	};
}