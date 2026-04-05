#include "../SamplerResourceService.h"
#include "../../Common/LogService.h"
#include "../../Engine.h"

using namespace Inno;

bool SamplerResourceService::Setup(IServiceConfig* systemConfig)
{
	m_Pool.Initialize(256);
	m_ObjectStatus = ObjectStatus::Activated;
	Log(Success, "SamplerResourceService Setup finished.");
	return true;
}

bool SamplerResourceService::Terminate()
{
	m_Pool.Terminate();
	m_ObjectStatus = ObjectStatus::Terminated;
	Log(Success, "SamplerResourceService has been terminated.");
	return true;
}

SamplerComponent* SamplerResourceService::Add(const char* name)
{
	return m_Pool.Allocate(name);
}

bool SamplerResourceService::Delete(SamplerComponent* ptr)
{
	m_Pool.Release(ptr);
	return true;
}

void SamplerResourceService::Initialize(SamplerComponent* sampler)
{
	InitializeImpl(sampler);
}
