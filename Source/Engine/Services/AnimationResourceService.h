#pragma once
#include "../Interface/IService.h"
#include "../Component/SkeletonComponent.h"
#include "../Component/AnimationComponent.h"
#include "../Common/EntityID.h"

namespace Inno
{
	struct AnimationResourceServiceImpl;
	class AnimationResourceService : public IService
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(AnimationResourceService);

		bool Setup(IServiceConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;
		ObjectStatus GetStatus() override;

		SkeletonComponent*  AddSkeletonComponent();
		AnimationComponent* AddAnimationComponent();

		bool InitializeSkeletonComponent(SkeletonComponent* rhs);
		bool InitializeAnimationComponent(AnimationComponent* rhs);

	private:
		AnimationResourceServiceImpl* m_Impl;
	};
}
