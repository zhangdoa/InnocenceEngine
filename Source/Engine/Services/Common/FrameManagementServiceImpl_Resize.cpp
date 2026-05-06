#include "../FrameManagementService.h"
#include "../GraphicsHardwareService.h"
#include "../RenderPassResourceService.h"

#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

#include "../../Services/RenderingConfigurationService.h"

#include "../../Engine.h"

using namespace Inno;

bool FrameManagementService::Present()
{
	PresentImpl();

	if (m_needResize)
	{
		m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
		m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
		m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Copy);

		auto l_graphicsSemaphoreValue = m_HardwareService->GetSemaphoreValue(GPUEngineType::Graphics);
		auto l_computeSemaphoreValue = m_HardwareService->GetSemaphoreValue(GPUEngineType::Compute);
		auto l_copySemaphoreValue = m_HardwareService->GetSemaphoreValue(GPUEngineType::Copy);

		m_HardwareService->WaitOnCPU(l_graphicsSemaphoreValue, GPUEngineType::Graphics);
		m_HardwareService->WaitOnCPU(l_computeSemaphoreValue, GPUEngineType::Compute);
		m_HardwareService->WaitOnCPU(l_copySemaphoreValue, GPUEngineType::Copy);

		ExecuteResize();

		m_needResize = false;
	}

	return true;
}

bool FrameManagementService::WaitForGPUIdle()
{
	Log(Verbose, "WaitForGPUIdle: signaling all queues...");
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Graphics);
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Compute);
	m_HardwareService->SignalOnGPU(m_GlobalSemaphore, GPUEngineType::Copy);

	Log(Verbose, "WaitForGPUIdle: waiting on Graphics...");
	m_HardwareService->WaitOnCPU(m_HardwareService->GetSemaphoreValue(GPUEngineType::Graphics), GPUEngineType::Graphics);
	Log(Verbose, "WaitForGPUIdle: waiting on Compute...");
	m_HardwareService->WaitOnCPU(m_HardwareService->GetSemaphoreValue(GPUEngineType::Compute), GPUEngineType::Compute);
	Log(Verbose, "WaitForGPUIdle: waiting on Copy...");
	m_HardwareService->WaitOnCPU(m_HardwareService->GetSemaphoreValue(GPUEngineType::Copy), GPUEngineType::Copy);
	Log(Verbose, "WaitForGPUIdle: complete.");

	return true;
}

bool FrameManagementService::Resize()
{
	Log(Success, "FrameManagementService::Resize requested.");
	m_needResize = true;
	return true;
}

bool FrameManagementService::ExecuteResize()
{
	Log(Success, "FrameManagementService::ExecuteResize begin.");
	PreResize();
	ResizeImpl();
	PostResize();
	Log(Success, "FrameManagementService::ExecuteResize complete.");

	return true;
}

bool FrameManagementService::PreResize()
{
	bool l_result = true;

	g_Engine->Get<RenderPassResourceService>()->ForEach([&](RenderPassComponent* i)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			return;
		if (!PreResize(i))
		{
			Log(Error, "Can't delete resources for ", i->m_InstanceName, " when resizing.");
			l_result = false;
		}
	});

	return l_result;
}

bool FrameManagementService::PreResize(RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	g_Engine->Get<RenderPassResourceService>()->DeleteRenderTargets(renderPass);

	return true;
}

bool FrameManagementService::PostResize()
{
	auto l_screenResolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	bool l_result = true;

	g_Engine->Get<RenderPassResourceService>()->ForEach([&](RenderPassComponent* i)
	{
		if (i->m_ObjectStatus != ObjectStatus::Activated)
			return;
		if (!PostResize(l_screenResolution, i))
		{
			Log(Error, "Can't resize ", i->m_InstanceName);
			l_result = false;
		}
	});

	return l_result;
}

bool FrameManagementService::PostResize(const TVec2<uint32_t>& screenResolution, RenderPassComponent* renderPass)
{
	if (!renderPass->m_RenderPassDesc.m_Resizable)
		return true;

	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Width = screenResolution.x;
	renderPass->m_RenderPassDesc.m_RenderTargetDesc.Height = screenResolution.y;

	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)screenResolution.x;
	renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)screenResolution.y;

	auto l_rpService = g_Engine->Get<RenderPassResourceService>();
	l_rpService->CreateOutputMergerTargets(renderPass);
	l_rpService->InitializeOutputMergerTargets(renderPass);
	l_rpService->OnOutputMergerTargetsCreated(renderPass);

	renderPass->m_PipelineStateObject = l_rpService->AddPipelineStateObject();

	l_rpService->CreatePipelineStateObject(renderPass);

	if (renderPass->m_OnResize)
		renderPass->m_OnResize();

	return true;
}
