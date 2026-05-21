#include "SSRCSpatialVerticalPass.h"
#include "ScreenTileConstants.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "OpaquePass.h"
#include "SSRCTemporalPass.h"
#include "SSRCSpatialHorizontalPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool SSRCSpatialVerticalPass::Setup(IServiceConfig* systemConfig)
{
	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SSRCSpatialVerticalPass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "SSRCSpatialVertical.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SSRCSpatialVerticalPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&SSRCSpatialVerticalPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// TASK-182: opt in to clear-on-bypass. m_Result is what LightPass reads
	// for the GI contribution; without clearing on bypass LightPass reads
	// stale last-frame GI and the RasterizedGI=false toggle leaves visible
	// frozen GI on screen. RecordClearCommandList below records the clear.
	m_ClearOnBypass = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(6);

	// b0 - PerFrame CBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// t0 - G-buffer world position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;

	// t1 - G-buffer world normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;

	// t2 - horizontal scratch from SSRCSpatialHorizontalPass
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ComputeOnly;

	// t3 - blur mask (R Float16) — same texture both passes consume
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ComputeOnly;

	// u0 - final per-pixel irradiance (rgb / max(N, 1), 1) for LightPass
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ComputeOnly;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("SSRCSpatialVerticalPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("SSRCSpatialVerticalPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool SSRCSpatialVerticalPass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool SSRCSpatialVerticalPass::Terminate()
{
	g_Engine->Get<TextureResourceService>()->Delete(m_Result);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);
	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus SSRCSpatialVerticalPass::GetStatus() { return m_ObjectStatus; }

bool SSRCSpatialVerticalPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (!m_Result || m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCB = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	auto l_HInput = SSRCSpatialHorizontalPass::Get().GetResult();
	auto l_BlurMask = SSRCTemporalPass::Get().GetBlurMask();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(l_HInput, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(l_BlurMask, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCB, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_HInput, 3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_BlurMask, 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 5);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, uint32_t(l_viewportSize.x / static_cast<float>(ScreenTile::SCREEN_TILE_SIZE)), uint32_t(l_viewportSize.y / static_cast<float>(ScreenTile::SCREEN_TILE_SIZE)), 1);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

// TASK-182 — clear-on-bypass path. Mirrors PrepareCommandList's CL
// lifecycle so the dispatch site's two-CL Execute / Signal pattern still
// drains as in the live path: graphics CL transitions m_Result to
// WriteOnly (UAV), compute CL issues a UAV-clear of the texture's
// declared ClearColor (zero by default), CLs are Closed and Executed by
// the dispatch site. m_Result's state-tracker ends in WriteOnly, which
// LightPass then transitions to ReadOnly per its existing flow — no
// barrier-state divergence between live and clear paths.
bool SSRCSpatialVerticalPass::RecordClearCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (!m_Result || m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_textureService = g_Engine->Get<TextureResourceService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_textureService->Clear(m_CommandListComp_Compute, m_Result);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

RenderPassComponent* SSRCSpatialVerticalPass::GetRenderPassComp() { return m_RenderPassComp; }
TextureComponent* SSRCSpatialVerticalPass::GetResult() { return m_Result; }

bool SSRCSpatialVerticalPass::RenderTargetsCreationFunc()
{
	if (m_Result)
		g_Engine->Get<TextureResourceService>()->Delete(m_Result);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	m_Result = g_Engine->Get<TextureResourceService>()->Add("SSRCSpatialVerticalPass Result");
	m_Result->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_Result->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	g_Engine->Get<TextureResourceService>()->Initialize(m_Result);
	return true;
}
