#include "AnimationService.h"

#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Common/DoubleBuffer.h"
#include "../Common/Timer.h"
#include "../Common/TaskScheduler.h"

#include "EntityRegistry.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct AnimationServiceImpl
	{
		AnimationServiceImpl();

		void initializeAnimation(AnimationComponent* rhs);
		AnimationData getAnimationData(const char* animationName);
		void simulateAnimation();

		ThreadSafeUnorderedMap<EntityID, AnimationInstance> m_AnimationInstanceMap;
		ThreadSafeUnorderedMap<std::string, AnimationData> m_AnimationDataInfosLUT;
		ThreadSafeQueue<AnimationComponent*> m_UninitializedAnimations;

		int64_t m_PreviousTime = 0;
		int64_t m_CurrentTime = 0;

		uint32_t m_SkeletonCount = 0;
		uint32_t m_AnimationCount = 0;

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}

AnimationServiceImpl::AnimationServiceImpl()
{
	m_PreviousTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	m_CurrentTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
}

void AnimationServiceImpl::initializeAnimation(AnimationComponent* rhs)
{
	std::string l_name = rhs->m_InstanceName.c_str();

	auto l_keyData = g_Engine->getRenderingServer()->AddGPUBufferComponent((l_name + "_KeyData").c_str());
	l_keyData->m_ElementCount = rhs->m_KeyData.capacity();
	l_keyData->m_ElementSize = sizeof(KeyData);
	l_keyData->m_GPUAccessibility = Accessibility::ReadWrite;

	g_Engine->getRenderingServer()->Initialize(l_keyData);
	g_Engine->getRenderingServer()->Upload(l_keyData, &rhs->m_KeyData[0]);

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	AnimationData l_info;
	l_info.ADC = rhs;
	l_info.keyData = l_keyData;

	m_AnimationDataInfosLUT.emplace(rhs->m_InstanceName.c_str(), l_info);
}

AnimationData AnimationServiceImpl::getAnimationData(const char* animationName)
{
	auto l_result = m_AnimationDataInfosLUT.find(animationName);
	if (l_result != m_AnimationDataInfosLUT.end())
	{
		return l_result->second;
	}
	else
	{
		return AnimationData();
	}
}

void AnimationServiceImpl::simulateAnimation()
{
	m_CurrentTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	float l_tickTime = float(m_CurrentTime - m_PreviousTime) / 1000.0f;

	if (m_AnimationInstanceMap.size())
	{
		for (auto& i : m_AnimationInstanceMap)
		{
			if (!i.second.isFinished)
			{
				if (i.second.currentTime < i.second.animationData.ADC->m_Duration)
				{
					i.second.currentTime += l_tickTime / 60.0f;
				}
				else
				{
					if (i.second.isLooping)
					{
						i.second.currentTime -= i.second.animationData.ADC->m_Duration;
					}
					else
					{
						i.second.isFinished = true;
					}
				}
			}
		}

		m_AnimationInstanceMap.erase_if([](auto it) { return it.second.isFinished; });
	}

	m_PreviousTime = m_CurrentTime;
}

bool AnimationService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new AnimationServiceImpl();

	m_Impl->m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool AnimationService::Initialize()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool AnimationService::Update()
{
	m_Impl->simulateAnimation();

	while (m_Impl->m_UninitializedAnimations.size() > 0)
	{
		AnimationComponent* l_Animations;
		m_Impl->m_UninitializedAnimations.tryPop(l_Animations);

		if (l_Animations)
		{
			m_Impl->initializeAnimation(l_Animations);
		}
	}
	
	return true;
}

bool AnimationService::Terminate()
{
	delete m_Impl;
	return true;
}

ObjectStatus AnimationService::GetStatus()
{
	return 	m_Impl->m_ObjectStatus;
}

SkeletonComponent* AnimationService::AddSkeletonComponent()
{
	auto l_Entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Persistence, ("Skeleton_" + std::to_string(m_Impl->m_SkeletonCount) + "/").c_str());
	auto& l_SDC = g_Engine->Get<EntityRegistry>()->Emplace<SkeletonComponent>(l_Entity);
	m_Impl->m_SkeletonCount++;
	return &l_SDC;
}

AnimationComponent* AnimationService::AddAnimationComponent()
{
	auto l_Entity = g_Engine->Get<EntityRegistry>()->Spawn(ObjectLifespan::Persistence, ("Animation_" + std::to_string(m_Impl->m_AnimationCount) + "/").c_str());
	auto& l_ADC = g_Engine->Get<EntityRegistry>()->Emplace<AnimationComponent>(l_Entity);
	m_Impl->m_AnimationCount++;
	return &l_ADC;
}

bool AnimationService::InitializeSkeletonComponent(SkeletonComponent* rhs)
{
	// TODO Phase2-migrate: rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool AnimationService::InitializeAnimationComponent(AnimationComponent* rhs)
{
	m_Impl->m_UninitializedAnimations.push(rhs);

	return true;
}

bool AnimationService::PlayAnimation(EntityID Entity, const char* AnimationName, bool IsLooping)
{
	auto l_AnimationData = m_Impl->getAnimationData(AnimationName);

	if (l_AnimationData.ADC != nullptr)
	{
		AnimationInstance l_Instance;

		l_Instance.animationData = l_AnimationData;
		l_Instance.currentTime = 0.0f;
		l_Instance.isLooping = IsLooping;
		l_Instance.isFinished = false;

		m_Impl->m_AnimationInstanceMap.emplace(Entity, l_Instance);

		return true;
	}

	return false;
}

bool AnimationService::StopAnimation(EntityID Entity)
{
	auto l_Result = m_Impl->m_AnimationInstanceMap.find(Entity);
	if (l_Result != m_Impl->m_AnimationInstanceMap.end())
	{
		m_Impl->m_AnimationInstanceMap.erase(l_Result->first);

		return true;
	}

	return false;
}

AnimationInstance AnimationService::GetAnimationInstance(EntityID Entity)
{
	auto l_result = m_Impl->m_AnimationInstanceMap.find(Entity);
	if (l_result != m_Impl->m_AnimationInstanceMap.end())
	{
		return l_result->second;
	}
	else
	{
		return AnimationInstance();
	}
}