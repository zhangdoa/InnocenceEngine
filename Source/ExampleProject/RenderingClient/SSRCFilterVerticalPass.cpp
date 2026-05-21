#include "SSRCFilterVerticalPass.h"
#include "SSRCConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "SSRCFilterHorizontalPass.h"
#include "SSRCReprojectionPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool SSRCFilterVerticalPass::Setup(IServiceConfig* systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SSRCFilterVerticalPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "SSRCFilterVertical.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SSRCFilterVerticalPass");
	
	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&SSRCFilterVerticalPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(6);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	// t0 - horizontal filtered input
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	// t1 - probe positions
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Compute;

	// t2 - probe normals
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Compute;

	// t3 - probe mask (GI-1.0 §2.1.5)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Compute;

	// u0 - final filtered output
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("SSRCFilterVerticalPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("SSRCFilterVerticalPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SSRCFilterVerticalPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SSRCFilterVerticalPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<TextureResourceService>()->Delete(m_Result);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SSRCFilterVerticalPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SSRCFilterVerticalPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_horizontalResult = SSRCFilterHorizontalPass::Get().GetResult();
	if (l_horizontalResult->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	// Use graphics command list to transition resources
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(l_horizontalResult, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(SSRCReprojectionPass::Get().GetCurrentProbePosition()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(SSRCReprojectionPass::Get().GetCurrentProbeNormal()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(SSRCReprojectionPass::Get().GetProbeMask()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_horizontalResult, 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSRCReprojectionPass::Get().GetCurrentProbePosition(), 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSRCReprojectionPass::Get().GetCurrentProbeNormal(), 3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSRCReprojectionPass::Get().GetProbeMask(), 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 5);

	auto dispatch_x = SSRC::TileCount(l_horizontalResult->m_TextureDesc.Width);
	auto dispatch_y = SSRC::TileCount(l_horizontalResult->m_TextureDesc.Height);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, dispatch_x, dispatch_y, 1);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SSRCFilterVerticalPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* SSRCFilterVerticalPass::GetResult()
{
	return m_Result;
}

bool SSRCFilterVerticalPass::RenderTargetsCreationFunc()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_horizontalResult = SSRCFilterHorizontalPass::Get().GetResult();

	if (m_Result)
		g_Engine->Get<TextureResourceService>()->Delete(m_Result);

	m_Result = g_Engine->Get<TextureResourceService>()->Add("SSRCFilterVerticalPass_Result");
	m_Result->m_TextureDesc = l_horizontalResult->m_TextureDesc;
	g_Engine->Get<TextureResourceService>()->Initialize(m_Result);

	return true;
}