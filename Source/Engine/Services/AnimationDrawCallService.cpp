#include "AnimationDrawCallService.h"
#include "../Common/LogService.h"
#include "../Engine.h"
#include "GraphicsResourceService.h"

using namespace Inno;

namespace Inno
{
	struct AnimationDrawCallServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		std::vector<AnimationDrawCallInfo> m_AnimationDrawCallInfoVector;
		std::vector<AnimationConstantBuffer> m_AnimationCBVector;

		GPUBufferComponent* m_AnimationGPUBufferComp;

		bool Setup(IServiceConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();

		bool UpdateDrawCalls();
	};
}

bool AnimationDrawCallServiceImpl::Setup(IServiceConfig* systemConfig)
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

	m_AnimationGPUBufferComp = l_rsService->AddGPUBufferComponent("AnimationCBuffer/");

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool AnimationDrawCallServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

		m_AnimationGPUBufferComp->m_ElementCount = 512;
		m_AnimationGPUBufferComp->m_ElementSize = sizeof(AnimationConstantBuffer);

		l_rsService->Initialize(m_AnimationGPUBufferComp);

		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "AnimationDrawCallService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "AnimationDrawCallService is not created!");
		return false;
	}
}

bool AnimationDrawCallServiceImpl::UpdateDrawCalls()
{
	m_AnimationDrawCallInfoVector.clear();
	m_AnimationCBVector.clear();

	return true;
}

bool AnimationDrawCallServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		UpdateDrawCalls();

		if (m_AnimationCBVector.size() > 0)
		{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
			l_rsService->Upload(m_AnimationGPUBufferComp, m_AnimationCBVector, 0, m_AnimationCBVector.size());
		}

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool AnimationDrawCallServiceImpl::Terminate()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();

	l_rsService->Delete(m_AnimationGPUBufferComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "AnimationDrawCallService has been terminated.");
	return true;
}

bool AnimationDrawCallService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new AnimationDrawCallServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool AnimationDrawCallService::Initialize()
{
	return m_Impl->Initialize();
}

bool AnimationDrawCallService::Update()
{
	return m_Impl->Update();
}

bool AnimationDrawCallService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus AnimationDrawCallService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const std::vector<AnimationDrawCallInfo>& AnimationDrawCallService::GetAnimationDrawCallInfo()
{
	return m_Impl->m_AnimationDrawCallInfoVector;
}

GPUBufferComponent* AnimationDrawCallService::GetAnimationBuffer()
{
	return m_Impl->m_AnimationGPUBufferComp;
}
