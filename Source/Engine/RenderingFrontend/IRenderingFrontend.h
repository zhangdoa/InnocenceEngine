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

		virtual bool runRayTrace() = 0;

		virtual MeshComponent* addMeshComponent() = 0;
		virtual TextureComponent* addTextureComponent() = 0;
		virtual MaterialComponent* addMaterialComponent() = 0;
		virtual SkeletonComponent* addSkeletonComponent() = 0;
		virtual AnimationComponent* addAnimationComponent() = 0;

		virtual bool registerMeshComponent(MeshComponent* rhs, bool AsyncUploadToGPU = true) = 0;
		virtual bool registerMaterialComponent(MaterialComponent* rhs, bool AsyncUploadToGPU = true) = 0;
		virtual bool registerSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU = true) = 0;
		virtual bool registerAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU = true) = 0;

		virtual MeshComponent* getMeshComponent(ProceduralMeshShape shape) = 0;
		virtual TextureComponent* getTextureComponent(WorldEditorIconType iconType) = 0;
		virtual MaterialComponent* getDefaultMaterialComponent() = 0;

		virtual bool transferDataToGPU() = 0;

		virtual TVec2<uint32_t> getScreenResolution() = 0;
		virtual bool setScreenResolution(TVec2<uint32_t> screenResolution) = 0;

		virtual RenderingConfig getRenderingConfig() = 0;
		virtual bool setRenderingConfig(RenderingConfig renderingConfig) = 0;

		virtual RenderingCapability getRenderingCapability() = 0;

		virtual RenderPassDesc getDefaultRenderPassDesc() = 0;

		virtual bool playAnimation(VisibleComponent* rhs, const char* animationName, bool isLooping = false) = 0;
		virtual bool stopAnimation(VisibleComponent* rhs, const char* animationName) = 0;

		virtual const PerFrameConstantBuffer& getPerFrameConstantBuffer() = 0;
		virtual const std::vector<CSMConstantBuffer>& getCSMConstantBuffer() = 0;
		virtual const std::vector<PointLightConstantBuffer>& getPointLightConstantBuffer() = 0;
		virtual const std::vector<SphereLightConstantBuffer>& getSphereLightConstantBuffer() = 0;

		virtual const std::vector<DrawCallInfo>& getDrawCallInfo() = 0;
		virtual const std::vector<PerObjectConstantBuffer>& getPerObjectConstantBuffer() = 0;
		virtual const std::vector<MaterialConstantBuffer>& getMaterialConstantBuffer() = 0;

		virtual const std::vector<AnimationDrawCallInfo>& getAnimationDrawCallInfo() = 0;
		virtual const std::vector<AnimationConstantBuffer>& getAnimationConstantBuffer() = 0;

		virtual const std::vector<BillboardPassDrawCallInfo>& getBillboardPassDrawCallInfo() = 0;
		virtual const std::vector<PerObjectConstantBuffer>& getBillboardPassPerObjectConstantBuffer() = 0;

		virtual const std::vector<DebugPassDrawCallInfo>& getDebugPassDrawCallInfo() = 0;
		virtual const std::vector<PerObjectConstantBuffer>& getDebugPassPerObjectConstantBuffer() = 0;
	};
}