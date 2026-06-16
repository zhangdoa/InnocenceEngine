#include "../RenderPassResourceService.h"
#include "../../Common/LogService.h"

using namespace Inno;

bool RenderPassResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Pool.Initialize(128);
	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool RenderPassResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

RenderPassComponent* RenderPassResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

RenderPassComponent* RenderPassResourceService::Find(const char* name)
{
	return m_Pool.Find(name);
}

bool RenderPassResourceService::Delete(RenderPassComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void RenderPassResourceService::ForEach(std::function<void(RenderPassComponent*)> func)
{
	m_Pool.ForEach(func);
}

void RenderPassResourceService::Initialize(RenderPassComponent* renderPass)
{
	if (renderPass->m_ObjectStatus == ObjectStatus::Activated)
		return;

	m_DeferredQueue.push(renderPass);
}

bool RenderPassResourceService::InitializeComponents()
{
	RenderPassComponent* l_renderPass = nullptr;
	while (m_DeferredQueue.tryPop(l_renderPass))
	{
		if (!l_renderPass)
			continue;

		if (InitializeRenderPass(l_renderPass))
		{
			l_renderPass->m_ObjectStatus = ObjectStatus::Activated;
		}
		else
		{
			Log(Error, " ", l_renderPass->m_InstanceName.c_str(), " failed to initialize; pass suspended.");
			l_renderPass->m_ObjectStatus = ObjectStatus::Suspended;
		}
	}
	return true;
}
