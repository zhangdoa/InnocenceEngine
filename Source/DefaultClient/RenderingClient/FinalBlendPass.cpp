#include "FinalBlendPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "BillboardPass.h"
#include "DebugPass.h"
#include "LuminanceAveragePass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool FinalBlendPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("FinalBlendPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "finalBlendPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("FinalBlendPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&FinalBlendPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(6);
	
	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;
	// @TODO: maybe it's wrong to write like this
	//m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessFunc = std::bind(&RenderingContextService::GetGPUBufferComponent, g_Engine->Get<RenderingContextService>(), GPUBufferUsageType::PerFrame);

	// t0 - Input
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;
	// @TODO: need to figure out a proper way to pass it through
	//m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessFunc = std::bind(&FinalBlendPass::GetContext, this);

	// t1 - BillboardPass
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Compute;
	//m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessFunc = std::bind(&BillboardPass::GetResult, BillboardPass::Get());

	// t2 - DebugPass
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Compute;
	//m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessFunc = std::bind(&DebugPass::GetResult, DebugPass::Get());

	// u0 - LuminanceAveragePass
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Compute;
	//m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessFunc = std::bind(&LuminanceAveragePass::GetResult, LuminanceAveragePass::Get());

	// u1 - Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Compute;
	//m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessFunc = std::bind(&FinalBlendPass::GetResult, this);

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = l_renderingServer->AddCommandListComponent("FinalBlendPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = l_renderingServer->AddCommandListComponent("FinalBlendPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool FinalBlendPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_CommandListComp_Graphics);
	l_renderingServer->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool FinalBlendPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_Result);
	l_renderingServer->Delete(m_CommandListComp_Compute);
	l_renderingServer->Delete(m_CommandListComp_Graphics);
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);
	
	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus FinalBlendPass::GetStatus()
{
	return m_ObjectStatus;
}

bool FinalBlendPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_luminanceAverage = LuminanceAveragePass::Get().GetResult();
	if (l_luminanceAverage->m_ObjectStatus != ObjectStatus::Activated)
		return false;	

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_renderingContext = reinterpret_cast<FinalBlendPassRenderingContext*>(renderingContext);
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	// Use graphics command list to transition resources
	l_renderingServer->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_renderingServer->TryToTransitState(reinterpret_cast<TextureComponent*>(l_renderingContext->m_input), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	// Don't assume source state - let TryToTransitState use actual current state
	l_renderingServer->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadWrite, Accessibility::WriteOnly);
	l_renderingServer->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_renderingServer->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Compute);

	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_renderingContext->m_input, 1);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, BillboardPass::Get().GetRenderPassComp()->m_RenderTargets[0], 2);
	// l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, DebugPass::Get().GetRenderPassComp()->m_RenderTargets[0], 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_luminanceAverage, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 5);

	l_renderingServer->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->UnbindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_renderingContext->m_input, 0);

	l_renderingServer->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;	

	return true;
}

RenderPassComponent* FinalBlendPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* FinalBlendPass::GetResult()
{
	return m_Result;
}

bool FinalBlendPass::RenderTargetsCreationFunc()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_Result)
		l_renderingServer->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	m_Result = l_renderingServer->AddTextureComponent("Final Blend Pass Result/");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ComputeOnly;

	l_renderingServer->Initialize(m_Result);

	return true;
}