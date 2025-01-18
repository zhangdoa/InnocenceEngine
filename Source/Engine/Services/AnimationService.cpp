#include "AnimationService.h"

#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/ThreadSafeQueue.h"
#include "../Common/DoubleBuffer.h"
#include "../Common/Timer.h"
#include "../Common/TaskScheduler.h"

#include "EntityManager.h"
#include "ComponentManager.h"

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

		ThreadSafeUnorderedMap<uint64_t, AnimationInstance> m_animationInstanceMap;
		ThreadSafeUnorderedMap<std::string, AnimationData> m_animationDataInfosLUT;
		ThreadSafeQueue<AnimationComponent*> m_uninitializedAnimations;

		int64_t m_previousTime = 0;
		int64_t m_currentTime = 0;

		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}

AnimationServiceImpl::AnimationServiceImpl()
{
	m_previousTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	m_currentTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
}

void AnimationServiceImpl::initializeAnimation(AnimationComponent* rhs)
{
	std::string l_name = rhs->m_InstanceName.c_str();

	auto l_keyData = g_Engine->getRenderingServer()->AddGPUBufferComponent((l_name + "_KeyData").c_str());
	l_keyData->m_Owner = rhs->m_Owner;
	l_keyData->m_ElementCount = rhs->m_KeyData.capacity();
	l_keyData->m_ElementSize = sizeof(KeyData);
	l_keyData->m_GPUAccessibility = Accessibility::ReadWrite;

	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(l_keyData);
	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_keyData, &rhs->m_KeyData[0]);

	rhs->m_ObjectStatus = ObjectStatus::Activated;

	AnimationData l_info;
	l_info.ADC = rhs;
	l_info.keyData = l_keyData;

	m_animationDataInfosLUT.emplace(rhs->m_InstanceName.c_str(), l_info);
}

AnimationData AnimationServiceImpl::getAnimationData(const char* animationName)
{
	auto l_result = m_animationDataInfosLUT.find(animationName);
	if (l_result != m_animationDataInfosLUT.end())
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
	m_currentTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);

	float l_tickTime = float(m_currentTime - m_previousTime) / 1000.0f;

	if (m_animationInstanceMap.size())
	{
		for (auto& i : m_animationInstanceMap)
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

		m_animationInstanceMap.erase_if([](auto it) { return it.second.isFinished; });
	}

	m_previousTime = m_currentTime;
}

bool AnimationService::Setup(ISystemConfig* systemConfig)
{	
	m_Impl = new AnimationServiceImpl();

	g_Engine->Get<ComponentManager>()->RegisterType<SkeletonComponent>(2048, this);
	g_Engine->Get<ComponentManager>()->RegisterType<AnimationComponent>(16384, this);

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

	while (m_Impl->m_uninitializedAnimations.size() > 0)
	{
		AnimationComponent* l_Animations;
		m_Impl->m_uninitializedAnimations.tryPop(l_Animations);

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
	static std::atomic<uint32_t> skeletonCount = 0;
	auto l_parentEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Persistence, ("Skeleton_" + std::to_string(skeletonCount) + "/").c_str());
	auto l_SDC = g_Engine->Get<ComponentManager>()->Spawn<SkeletonComponent>(l_parentEntity, false, ObjectLifespan::Persistence);
	l_SDC->m_Owner = l_parentEntity;
	l_SDC->m_Serializable = false;
	l_SDC->m_ObjectStatus = ObjectStatus::Created;
	l_SDC->m_ObjectLifespan = ObjectLifespan::Persistence;
	skeletonCount++;
	return l_SDC;
}

AnimationComponent* AnimationService::AddAnimationComponent()
{
	static std::atomic<uint32_t> animationCount = 0;
	auto l_parentEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Persistence, ("Animation_" + std::to_string(animationCount) + "/").c_str());
	auto l_ADC = g_Engine->Get<ComponentManager>()->Spawn<AnimationComponent>(l_parentEntity, false, ObjectLifespan::Persistence);
	l_ADC->m_Owner = l_parentEntity;
	l_ADC->m_Serializable = false;
	l_ADC->m_ObjectStatus = ObjectStatus::Created;
	l_ADC->m_ObjectLifespan = ObjectLifespan::Persistence;
	animationCount++;
	return l_ADC;
}

bool AnimationService::InitializeSkeletonComponent(SkeletonComponent* rhs, bool AsyncUploadToGPU)
{
	rhs->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool AnimationService::InitializeAnimationComponent(AnimationComponent* rhs, bool AsyncUploadToGPU)
{
	if (AsyncUploadToGPU)
	{
		m_Impl->m_uninitializedAnimations.push(rhs);
	}
	else
	{
		auto l_AnimationComponentInitializeTask = g_Engine->Get<TaskScheduler>()->Submit("AnimationComponentInitializeTask", 2,
			[=]() { m_Impl->initializeAnimation(rhs); });
		l_AnimationComponentInitializeTask->Wait();
	}

	return true;
}

bool AnimationService::PlayAnimation(ModelComponent* rhs, const char* animationName, bool isLooping)
{
	auto l_animationData = m_Impl->getAnimationData(animationName);

	if (l_animationData.ADC != nullptr)
	{
		AnimationInstance l_instance;

		l_instance.animationData = l_animationData;
		l_instance.currentTime = 0.0f;
		l_instance.isLooping = isLooping;
		l_instance.isFinished = false;

		m_Impl->m_animationInstanceMap.emplace(rhs->m_UUID, l_instance);

		return true;
	}

	return false;
}

bool AnimationService::StopAnimation(ModelComponent* rhs, const char* animationName)
{
	auto l_result = m_Impl->m_animationInstanceMap.find(rhs->m_UUID);
	if (l_result != m_Impl->m_animationInstanceMap.end())
	{
		m_Impl->m_animationInstanceMap.erase(l_result->first);

		return true;
	}

	return false;
}

AnimationInstance AnimationService::GetAnimationInstance(uint64_t UUID)
{
	auto l_result = m_Impl->m_animationInstanceMap.find(UUID);
	if (l_result != m_Impl->m_animationInstanceMap.end())
	{
		return l_result->second;
	}
	else
	{
		return AnimationInstance();
	}
}