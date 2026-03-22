#include "AnimationSimulationService.h"

#include "../Common/ThreadSafeUnorderedMap.h"
#include "../Common/Timer.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct AnimationSimulationServiceImpl
	{
		AnimationSimulationServiceImpl();

		void Tick();

		ThreadSafeUnorderedMap<std::string, AnimationData>  m_AnimationDataLUT;
		ThreadSafeUnorderedMap<EntityID, AnimationInstance> m_AnimationInstanceMap;

		int64_t      m_PreviousTime = 0;
		int64_t      m_CurrentTime  = 0;
		ObjectStatus m_ObjectStatus = ObjectStatus::Invalid;
	};
}

AnimationSimulationServiceImpl::AnimationSimulationServiceImpl()
{
	m_PreviousTime = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
	m_CurrentTime  = g_Engine->Get<Timer>()->GetCurrentTimeFromEpoch(TimeUnit::Millisecond);
}

void AnimationSimulationServiceImpl::Tick()
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

bool AnimationSimulationService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new AnimationSimulationServiceImpl();
	m_Impl->m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool AnimationSimulationService::Initialize()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool AnimationSimulationService::Update()
{
	m_Impl->Tick();
	return true;
}

bool AnimationSimulationService::Terminate()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Terminated;
	delete m_Impl;
	return true;
}

ObjectStatus AnimationSimulationService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

void AnimationSimulationService::RegisterAnimationData(const char* Name, AnimationData Data)
{
	m_Impl->m_AnimationDataLUT.emplace(Name, Data);
}

bool AnimationSimulationService::PlayAnimation(EntityID Entity, const char* AnimationName, bool IsLooping)
{
	auto l_result = m_Impl->m_AnimationDataLUT.find(AnimationName);
	if (l_result != m_Impl->m_AnimationDataLUT.end())
	{
		AnimationInstance l_Instance;
		l_Instance.animationData = l_result->second;
		l_Instance.currentTime   = 0.0f;
		l_Instance.isLooping     = IsLooping;
		l_Instance.isFinished    = false;

		m_Impl->m_AnimationInstanceMap.emplace(Entity, l_Instance);
		return true;
	}

	return false;
}

bool AnimationSimulationService::StopAnimation(EntityID Entity)
{
	auto l_result = m_Impl->m_AnimationInstanceMap.find(Entity);
	if (l_result != m_Impl->m_AnimationInstanceMap.end())
	{
		m_Impl->m_AnimationInstanceMap.erase(l_result->first);
		return true;
	}

	return false;
}

AnimationInstance AnimationSimulationService::GetAnimationInstance(EntityID Entity)
{
	auto l_result = m_Impl->m_AnimationInstanceMap.find(Entity);
	if (l_result != m_Impl->m_AnimationInstanceMap.end())
	{
		return l_result->second;
	}

	return AnimationInstance();
}
