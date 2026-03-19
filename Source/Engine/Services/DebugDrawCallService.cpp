#include "DebugDrawCallService.h"

#include "../Common/LogService.h"

using namespace Inno;

namespace Inno
{
	struct DebugDrawCallServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		mutable std::shared_mutex m_Mutex;

		std::vector<DebugPassDrawCallInfo> m_DebugPassDrawCallInfoVector;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();
	};
}

bool DebugDrawCallServiceImpl::Setup(ISystemConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool DebugDrawCallServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "DebugDrawCallService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "DebugDrawCallService is not created!");
		return false;
	}
}

bool DebugDrawCallServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		std::lock_guard<std::shared_mutex> l_lock(m_Mutex);

		m_DebugPassDrawCallInfoVector.clear();

		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool DebugDrawCallServiceImpl::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "DebugDrawCallService has been terminated.");
	return true;
}

bool DebugDrawCallService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new DebugDrawCallServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool DebugDrawCallService::Initialize()
{
	return m_Impl->Initialize();
}

bool DebugDrawCallService::Update()
{
	return m_Impl->Update();
}

bool DebugDrawCallService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus DebugDrawCallService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const std::vector<DebugPassDrawCallInfo>& DebugDrawCallService::GetDebugPassDrawCallInfo()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	return m_Impl->m_DebugPassDrawCallInfoVector;
}

void DebugDrawCallService::Submit(const DebugPassDrawCallInfo& info)
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_Mutex);
	m_Impl->m_DebugPassDrawCallInfoVector.emplace_back(info);
}
