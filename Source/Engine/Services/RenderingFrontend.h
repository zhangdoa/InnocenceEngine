#pragma once
#include "../Interface/ISystem.h"

#include "../RenderingServer/IRenderingServer.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"
#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"

#include "../Common/GPUDataStructure.h"

namespace Inno
{
	struct RenderingConfig
	{
		bool VSync = false;
		int32_t MSAAdepth = 4;
		bool useCSM = false;
		int32_t shadowMapResolution = 2048;
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
	
	struct AnimationData
	{
		AnimationComponent* ADC;
		GPUBufferComponent* keyData;
	};

	struct AnimationInstance
	{
		AnimationData animationData;
		float currentTime;
		bool isLooping;
		bool isFinished;
	};

	struct AnimationDrawCallInfo
	{
		AnimationInstance animationInstance;
		DrawCallInfo drawCallInfo;
		uint32_t animationConstantBufferIndex;
	};

	enum class WorldEditorIconType { DIRECTIONAL_LIGHT, POINT_LIGHT, SPHERE_LIGHT, UNKNOWN };

	class IRenderingFrontendConfig : public ISystemConfig
	{
	public:
		IRenderingServer* m_RenderingServer;
	};

	class RenderingFrontend : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(RenderingFrontend);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool RunRayTracing();

		MeshComponent* AddMeshComponent();
		TextureComponent* AddTextureComponent();
		MaterialComponent* AddMaterialComponent();
		SkeletonComponent* AddSkeletonComponent();
		AnimationComponent* AddAnimationComponent();

		bool InitializeMeshComponent(MeshComponent* rhs, bool AsyncUploadToGPU);
		bool InitializeMaterialComponent(MaterialComponent* rhs, bool AsyncUploadToGPU);
		bool InitializeSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU);
		bool InitializeAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU);

		MeshComponent* GetMeshComponent(ProceduralMeshShape shape);
		TextureComponent* GetTextureComponent(WorldEditorIconType iconType);
		MaterialComponent* GetDefaultMaterialComponent();

		bool TransferDataToGPU();

		TVec2<uint32_t> GetScreenResolution();
		bool SetScreenResolution(TVec2<uint32_t> screenResolution);

		RenderingConfig GetRenderingConfig();
		bool SetRenderingConfig(RenderingConfig renderingConfig);

		RenderingCapability GetRenderingCapability();

		RenderPassDesc GetDefaultRenderPassDesc();

		bool PlayAnimation(VisibleComponent* rhs, const char* animationName, bool isLooping);
		bool StopAnimation(VisibleComponent* rhs, const char* animationName);

		const PerFrameConstantBuffer& GetPerFrameConstantBuffer();
		const std::vector<CSMConstantBuffer>& GetCSMConstantBuffer();
		const std::vector<PointLightConstantBuffer>& GetPointLightConstantBuffer();
		const std::vector<SphereLightConstantBuffer>& GetSphereLightConstantBuffer();

		const std::vector<DrawCallInfo>& GetDrawCallInfo();
		const std::vector<PerObjectConstantBuffer>& GetPerObjectConstantBuffer();
		const std::vector<MaterialConstantBuffer>& GetMaterialConstantBuffer();

		const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo();
		const std::vector<AnimationConstantBuffer>& GetAnimationConstantBuffer();

		const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
		const std::vector<PerObjectConstantBuffer>& GetBillboardPassPerObjectConstantBuffer();

		const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo();
		const std::vector<PerObjectConstantBuffer>& GetDebugPassPerObjectConstantBuffer();
	};
}