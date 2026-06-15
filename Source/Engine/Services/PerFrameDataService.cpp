#include "PerFrameDataService.h"
#include "PerFrameDataService_Internal.h"
#include "PerFrameDataService_Toggles.h"
#include "FrameManagementService.h"
#include "../Engine.h"
using namespace Inno;

bool PerFrameDataService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new PerFrameDataServiceImpl();
	return m_Impl->Setup(systemConfig);
}

bool PerFrameDataService::Initialize()
{
	if (!m_Impl->Initialize())
		return false;
	RegisterDevTogglesForPerFrameData(*this);
	return true;
}

bool PerFrameDataService::Update()
{
	return m_Impl->Update();
}

bool PerFrameDataService::Terminate()
{
	auto result = m_Impl->Terminate();
	delete m_Impl;
	return result;
}

ObjectStatus PerFrameDataService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

const PerFrameConstantBuffer& PerFrameDataService::GetPerFrameConstantBuffer()
{
	return m_Impl->m_perFrameCBs[g_Engine->Get<FrameManagementService>()->GetCurrentFrame()];
}

GPUBufferComponent* PerFrameDataService::GetCurrentFrameBuffer()
{
	return m_Impl->GetCurrentFramePerFrameBuffer();
}

GPUBufferComponent* PerFrameDataService::GetPreviousFrameBuffer()
{
	return m_Impl->GetPreviousFramePerFrameBuffer();
}

void PerFrameDataService::SetDebugViewMode(DebugViewMode in_Mode)
{
	m_Impl->m_DebugViewMode.store(static_cast<uint32_t>(in_Mode), std::memory_order_relaxed);
}

DebugViewMode PerFrameDataService::GetDebugViewMode() const
{
	return static_cast<DebugViewMode>(m_Impl->m_DebugViewMode.load(std::memory_order_relaxed));
}

void PerFrameDataService::SetPointShadowBypass(bool in_Bypass)
{
	m_Impl->m_PointShadowBypass.store(in_Bypass ? 1u : 0u, std::memory_order_relaxed);
}

bool PerFrameDataService::GetPointShadowBypass() const
{
	return m_Impl->m_PointShadowBypass.load(std::memory_order_relaxed) != 0u;
}
