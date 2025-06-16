#pragma once
#include "../Interface/ISystem.h"

#include "../RenderingServer/IRenderingServer.h"

#include "../Component/MeshComponent.h"
#include "../Component/TextureComponent.h"
#include "../Component/MaterialComponent.h"

#include "../Common/GPUDataStructure.h"

#include "AnimationService.h"

namespace Inno
{
	enum class GPUBufferUsageType
	{
		PerFrame,
		Mesh,
		Material,
		PointLight,
		SphereLight,
		CSM,
		GI,
		Animation,
		Billboard
	};

	struct AnimationDrawCallInfo
	{
		AnimationInstance animationInstance;
		DrawCallInfo drawCallInfo;
		uint32_t animationConstantBufferIndex;
	};

	struct RenderingContextServiceImpl;
	class RenderingContextService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(RenderingContextService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		GPUBufferComponent* GetGPUBufferComponent(GPUBufferUsageType usageType);

		const PerFrameConstantBuffer& GetPerFrameConstantBuffer();

		const std::vector<DrawCallInfo>& GetDrawCallInfo();
		const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
		const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo();
		const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo();

	private:
		RenderingContextServiceImpl* m_Impl;
	};
}