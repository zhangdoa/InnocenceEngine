#pragma once
#include "../Interface/ISystem.h"
#include "../Component/AnimationComponent.h"
#include "../Common/EntityID.h"

namespace Inno { struct GPUBufferComponent; }

namespace Inno
{
	struct AnimationData
	{
		AnimationComponent* ADC    = nullptr;
		GPUBufferComponent* keyData = nullptr;
	};

	struct AnimationInstance
	{
		AnimationData animationData;
		float         currentTime = 0.f;
		bool          isLooping   = false;
		bool          isFinished  = false;
	};

	struct AnimationSimulationServiceImpl;
	class AnimationSimulationService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AnimationSimulationService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		void RegisterAnimationData(const char* Name, AnimationData Data);

		bool              PlayAnimation(EntityID Entity, const char* AnimationName, bool IsLooping);
		bool              StopAnimation(EntityID Entity);
		AnimationInstance GetAnimationInstance(EntityID Entity);

	private:
		AnimationSimulationServiceImpl* m_Impl;
	};
}
