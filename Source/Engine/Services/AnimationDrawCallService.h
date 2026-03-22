#pragma once
#include "../Interface/ISystem.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/GPUDataStructure.h"
#include "AnimationSimulationService.h"

namespace Inno
{
	struct AnimationDrawCallInfo
	{
		AnimationInstance animationInstance;
		uint32_t modelDataIndex;
		uint32_t animationConstantBufferIndex;
	};

	struct AnimationDrawCallServiceImpl;
	class AnimationDrawCallService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AnimationDrawCallService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		const std::vector<AnimationDrawCallInfo>& GetAnimationDrawCallInfo();
		GPUBufferComponent* GetAnimationBuffer();

	private:
		AnimationDrawCallServiceImpl* m_Impl;
	};
}
