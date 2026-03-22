#include "AnimationResourceService.h"
#include "AnimationSimulationService.h"
#include "../RenderingServer/IRenderingServer.h"
#include "../Common/ThreadSafeQueue.h"
#include "EntityRegistry.h"
#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct AnimationResourceServiceImpl
	{
		void InitializeAnimation(AnimationComponent* rhs);

		ThreadSafeQueue<AnimationComponent*> m_UninitializedAnimations;

		uint32_t     m_SkeletonCount   = 0;
		uint32_t     m_AnimationCount  = 0;
		ObjectStatus m_ObjectStatus    = ObjectStatus::Invalid;
	};
}

void AnimationResourceServiceImpl::InitializeAnimation(AnimationComponent* rhs)
{
	std::string l_name = rhs->m_InstanceName.c_str();

	auto l_keyData = g_Engine->getRenderingServer()->AddGPUBufferComponent((l_name + "_KeyData").c_str());
	l_keyData->m_ElementCount    = rhs->m_KeyData.capacity();
	l_keyData->m_ElementSize     = sizeof(KeyData);
	l_keyData->m_GPUAccessibility = Accessibility::ReadWrite;

	g_Engine->getRenderingServer()->Initialize(l_keyData);
	g_Engine->getRenderingServer()->Upload(l_keyData, &rhs->m_KeyData[0]);

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	AnimationData l_data;
	l_data.ADC     = rhs;
	l_data.keyData = l_keyData;

	g_Engine->Get<AnimationSimulationService>()->RegisterAnimationData(rhs->m_InstanceName.c_str(), l_data);
}

bool AnimationResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new AnimationResourceServiceImpl();
	m_Impl->m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool AnimationResourceService::Initialize()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool AnimationResourceService::Update()
{
	while (m_Impl->m_UninitializedAnimations.size() > 0)
	{
		AnimationComponent* l_Animation;
		m_Impl->m_UninitializedAnimations.tryPop(l_Animation);

		if (l_Animation)
		{
			m_Impl->InitializeAnimation(l_Animation);
		}
	}

	return true;
}

bool AnimationResourceService::Terminate()
{
	delete m_Impl;
	return true;
}

ObjectStatus AnimationResourceService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

SkeletonComponent* AnimationResourceService::AddSkeletonComponent()
{
	auto l_Entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Persistence, ("Skeleton_" + std::to_string(m_Impl->m_SkeletonCount) + "/").c_str());
	auto& l_SDC = g_Engine->Get<EntityRegistry>()->Emplace<SkeletonComponent>(l_Entity);
	m_Impl->m_SkeletonCount++;
	return &l_SDC;
}

AnimationComponent* AnimationResourceService::AddAnimationComponent()
{
	auto l_Entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Persistence, ("Animation_" + std::to_string(m_Impl->m_AnimationCount) + "/").c_str());
	auto& l_ADC = g_Engine->Get<EntityRegistry>()->Emplace<AnimationComponent>(l_Entity);
	m_Impl->m_AnimationCount++;
	return &l_ADC;
}

bool AnimationResourceService::InitializeSkeletonComponent(SkeletonComponent* rhs)
{
	return true;
}

bool AnimationResourceService::InitializeAnimationComponent(AnimationComponent* rhs)
{
	m_Impl->m_UninitializedAnimations.push(rhs);
	return true;
}
