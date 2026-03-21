#pragma once
#include "../Interface/ISystem.h"

#include "../RenderingServer/IRenderingServer.h"

#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"
#include "../Common/EntityID.h"

#include "../Common/GPUDataStructure.h"

namespace Inno
{
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

	struct AnimationServiceImpl;
	class AnimationService : public ISystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AnimationService);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		SkeletonComponent* AddSkeletonComponent();
		AnimationComponent* AddAnimationComponent();

		bool InitializeSkeletonComponent(SkeletonComponent* rhs);
		bool InitializeAnimationComponent(AnimationComponent* rhs);

		bool PlayAnimation(EntityID Entity, const char* AnimationName, bool IsLooping);
		bool StopAnimation(EntityID Entity);

		AnimationInstance GetAnimationInstance(EntityID Entity);

	private:
		AnimationServiceImpl* m_Impl;
	};
}