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

		const PerFrameConstantBuffer& GetPerFrameConstantBuffer();
		const std::vector<CSMConstantBuffer>& GetCSMConstantBuffer();
		const std::vector<PointLightConstantBuffer>& GetPointLightConstantBuffer();
		const std::vector<SphereLightConstantBuffer>& GetSphereLightConstantBuffer();

		const std::vector<DrawCallInfo>& GetDrawCallInfo();
		const std::vector<PerObjectConstantBuffer>& GetPerObjectConstantBuffer();
		const std::vector<MaterialConstantBuffer>& GetMaterialConstantBuffer();

		const std::vector<BillboardPassDrawCallInfo>& GetBillboardPassDrawCallInfo();
		const std::vector<PerObjectConstantBuffer>& GetBillboardPassPerObjectConstantBuffer();

		const std::vector<DebugPassDrawCallInfo>& GetDebugPassDrawCallInfo();
		const std::vector<PerObjectConstantBuffer>& GetDebugPassPerObjectConstantBuffer();

		const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo();
		const std::vector<AnimationConstantBuffer>& GetAnimationConstantBuffer();

	private:
		RenderingContextServiceImpl* m_Impl;
	};
}