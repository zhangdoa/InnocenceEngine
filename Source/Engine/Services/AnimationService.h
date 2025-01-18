#pragma once
#include "../Interface/ISystem.h"

#include "../RenderingServer/IRenderingServer.h"

#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"

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

		bool InitializeSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU);
		bool InitializeAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU);

		bool PlayAnimation(ModelComponent* rhs, const char* animationName, bool isLooping);
		bool StopAnimation(ModelComponent* rhs, const char* animationName);

		AnimationInstance GetAnimationInstance(uint64_t UUID);

	private:
		AnimationServiceImpl* m_Impl;
	};
}