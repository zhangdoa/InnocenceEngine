#include "LuminanceAveragePass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "LuminanceHistogramPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/GraphicsResourceService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

using namespace Inno;

bool LuminanceAveragePass::Setup(IServiceConfig* systemConfig)
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_Resizable = false;

	m_ShaderProgramComp = l_rsService->AddShaderProgramComponent("LuminanceAveragePass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "luminanceAveragePass.comp/";

	m_RenderPassComp = l_rsService->AddRenderPassComponent("LuminanceAveragePass/");

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// u0 - LuminanceHistogram
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	// u1 - LuminanceAverage
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = l_rsService->AddCommandListComponent("LuminanceAveragePass/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_luminanceAverage = l_rsService->AddGPUBufferComponent("LuminanceAverageGPUBuffer/");
	m_luminanceAverage->m_CPUAccessibility = Accessibility::Immutable;
	m_luminanceAverage->m_GPUAccessibility = Accessibility::ReadWrite;
	m_luminanceAverage->m_ElementCount = m_MaxResultToKeep;
	m_luminanceAverage->m_ElementSize = sizeof(float);

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool LuminanceAveragePass::Initialize()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_rsService->Initialize(m_ShaderProgramComp);
	l_rsService->Initialize(m_RenderPassComp);
	l_rsService->Initialize(m_CommandListComp_Compute);

	l_rsService->Initialize(m_luminanceAverage);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LuminanceAveragePass::Update()
{
	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

bool LuminanceAveragePass::Terminate()
{
	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	l_rsService->Delete(m_luminanceAverage);
	l_rsService->Delete(m_CommandListComp_Compute);
	l_rsService->Delete(m_RenderPassComp);
	l_rsService->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LuminanceAveragePass::GetStatus()
{
	return m_ObjectStatus;
}

bool LuminanceAveragePass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (LuminanceHistogramPass::Get().GetResult()->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_luminanceAverage->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_rsService = g_Engine->Get<GraphicsResourceService>();
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	l_hwService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_hwService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
	l_hwService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Compute);

	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, LuminanceHistogramPass::Get().GetResult(), 1);
	l_hwService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_luminanceAverage, 2);

	l_hwService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, 1, 1, 1);

	l_hwService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* LuminanceAveragePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* LuminanceAveragePass::GetResult()
{
	return m_luminanceAverage;
}