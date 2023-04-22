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
		int32_t shadowMapResolution = 1024;
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

	class IRenderingFrontend : public IComponentSystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingFrontend);

		virtual bool RunRayTracing() = 0;

		virtual MeshComponent* AddMeshComponent() = 0;
		virtual TextureComponent* AddTextureComponent() = 0;
		virtual MaterialComponent* AddMaterialComponent() = 0;
		virtual SkeletonComponent* AddSkeletonComponent() = 0;
		virtual AnimationComponent* AddAnimationComponent() = 0;

		virtual bool InitializeMeshComponent(MeshComponent* rhs, bool AsyncUploadToGPU = true) = 0;
		virtual bool InitializeMaterialComponent(MaterialComponent* rhs, bool AsyncUploadToGPU = true) = 0;
		virtual bool InitializeSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU = true) = 0;
		virtual bool InitializeAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU = true) = 0;

		virtual MeshComponent* GetMeshComponent(ProceduralMeshShape shape) = 0;
		virtual TextureComponent* GetTextureComponent(WorldEditorIconType iconType) = 0;
		virtual MaterialComponent* GetDefaultMaterialComponent() = 0;

		virtual bool TransferDataToGPU() = 0;

		virtual TVec2<uint32_t> GetScreenResolution() = 0;
		virtual bool SetScreenResolution(TVec2<uint32_t> screenResolution) = 0;

		virtual RenderingConfig GetRenderingConfig() = 0;
		virtual bool SetRenderingConfig(RenderingConfig renderingConfig) = 0;

		virtual RenderingCapability GetRenderingCapability() = 0;

		virtual RenderPassDesc GetDefaultRenderPassDesc() = 0;

		virtual bool PlayAnimation(VisibleComponent* rhs, const char* animationName, bool isLooping = false) = 0;
		virtual bool StopAnimation(VisibleComponent* rhs, const char* animationName) = 0;

		virtual const PerFrameConstantBuffer& GetPerFrameConstantBuffer() = 0;
		virtual const std::vector<CSMConstantBuffer>& GetCSMConstantBuffer() = 0;
		virtual const std::vector<PointLightConstantBuffer>& GetPointLightConstantBuffer() = 0;
		virtual const std::vector<SphereLightConstantBuffer>& GetSphereLightConstantBuffer() = 0;

		virtual const std::vector<DrawCallInfo>& GetDrawCallInfo() = 0;
		virtual const std::vector<PerObjectConstantBuffer>& GetPerObjectConstantBuffer() = 0;
		virtual const std::vector<MaterialConstantBuffer>& GetMaterialConstantBuffer() = 0;

		virtual const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo() = 0;
		virtual const std::vector<AnimationConstantBuffer>& GetAnimationConstantBuffer() = 0;

		virtual const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo() = 0;
		virtual const std::vector<PerObjectConstantBuffer>& GetBillboardPassPerObjectConstantBuffer() = 0;

		virtual const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo() = 0;
		virtual const std::vector<PerObjectConstantBuffer>& GetDebugPassPerObjectConstantBuffer() = 0;
	};
}