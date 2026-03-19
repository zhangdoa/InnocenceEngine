#include "RenderingContextService.h"

#include "../Common/LogService.h"

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct RenderingContextServiceImpl
	{
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		bool Setup(ISystemConfig* systemConfig);
		bool Initialize();
		bool Update();
		bool Terminate();
	};
}

bool RenderingContextServiceImpl::Setup(ISystemConfig* systemConfig)
{
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RenderingContextServiceImpl::Initialize()
{
	if (m_ObjectStatus == ObjectStatus::Created)
	{
		m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "RenderingContextService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "RenderingContextService is not created!");
		return false;
	}
}

bool RenderingContextServiceImpl::Update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		return true;
	}
	else
	{
		m_ObjectStatus = ObjectStatus::Suspended;
		return false;
	}
}

bool RenderingContextServiceImpl::Terminate()
{
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "RenderingContextService has been terminated.");
	return true;
}

bool RenderingContextService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new RenderingContextServiceImpl();

	return m_Impl->Setup(systemConfig);
}

bool RenderingContextService::Initialize()
{
	return m_Impl->Initialize();
}

bool RenderingContextService::Update()
{
	return m_Impl->Update();
}

bool RenderingContextService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus RenderingContextService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}
