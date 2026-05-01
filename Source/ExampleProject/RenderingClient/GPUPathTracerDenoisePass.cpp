#include "GPUPathTracerDenoisePass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "GPUPathTracerPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool GPUPathTracerDenoisePass::Setup(IServiceConfig* systemConfig)
{
	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("GPUPathTracerDenoisePass");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "GPUPathTracerDenoise.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("GPUPathTracerDenoisePass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseOutputMerger  = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// OnResize: m_Result is owned, not an output-merger target, so the
	// frame-management resize path skips it. Recreate at new resolution.
	// The PT pass also resizes its noisy + per-pixel hit UAVs; the engine
	// drains the GPU before the resize callbacks fire so the order between
	// PT's recreate and ours doesn't matter — both run before any frame
	// observes the new resolution.
	m_RenderPassComp->m_OnResize = [this]() { OnResize(); };

	// TASK-182 — clear-on-bypass. Active for runtime PT-primary toggles
	// (mid-flight ON→OFF): once the pass has activated at least once, a
	// subsequent bypass takes the clear-on-bypass path and m_Result is
	// zeroed, so any tooling that inspects the texture
	// (RenderTargetDebuggerPanel, ViewportSourceOverride) sees defined
	// zeros instead of stale denoise content. The boot-from-bypass case
	// (PT off from frame 0) is handled at the dispatch site by skipping
	// the entire pass — see ExampleRenderingClient.cpp PrepareCommands.
	m_ClearOnBypass = true;

	// Binding layout: b0=PerFrameCB, t0=NoisyRadiance, t1=PrimaryHitPos,
	//                 t2=PrimaryHitNormal, u0=HashGridKeys (RW UAV view),
	//                 u1=HashGridValue (RW UAV view), u2=DenoisedResult.
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(7);

	// b0 - PerFrame CB (set 0, binding 0)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	// t0 - PT noisy radiance (set 1, binding 0). Persistent ReadWrite UAV
	// on the producer side, transitioned to ReadOnly at the end of PT's
	// compute CL — this binding takes the SRV view.
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	// t1 - PrimaryHitPos
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Compute;

	// t2 - PrimaryHitNormal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Compute;

	// u0 - HashGridKeys (set 2, binding 0). UAV view on the same buffer
	// the PT raygen writes; the denoise pass only reads but matches the
	// writer's binding type so no SRV-view / state-transition dance is
	// needed (TASK-77.1.1 keeps the buffer persistently ReadWrite).
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Compute;

	// u1 - HashGridValue (persistent EMA buffer drained by the filter pass).
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Compute;

	// u2 - Denoised result (write target).
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerDenoisePass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("GPUPathTracerDenoisePass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool GPUPathTracerDenoisePass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	CreateResult();

	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool GPUPathTracerDenoisePass::Terminate()
{
	if (m_Result)
	{
		g_Engine->Get<TextureResourceService>()->Delete(m_Result);
		m_Result = nullptr;
	}
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus GPUPathTracerDenoisePass::GetStatus()
{
	return m_ObjectStatus;
}

bool GPUPathTracerDenoisePass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (!m_Result || m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	// Producer must be live and have built its UAVs. Bypassing on a not-
	// yet-activated PT pass keeps the dispatch matching the upstream's
	// own activation gate (PrepareCommands at ExampleRenderingClient.cpp:427).
	auto& l_pt = GPUPathTracerPass::Get();
	if (l_pt.GetStatus() != ObjectStatus::Activated)
		return false;

	auto* l_noisy   = static_cast<TextureComponent*>(l_pt.GetResult());
	auto* l_hitPos  = l_pt.GetPrimaryHitPosBuffer();
	auto* l_hitN    = l_pt.GetPrimaryHitNormalBuffer();
	auto* l_keys    = l_pt.GetHashGridKeys();
	auto* l_value   = l_pt.GetHashGridValue();
	if (!l_noisy  || l_noisy->m_ObjectStatus  != ObjectStatus::Activated) return false;
	if (!l_hitPos || l_hitPos->m_ObjectStatus != ObjectStatus::Activated) return false;
	if (!l_hitN   || l_hitN->m_ObjectStatus   != ObjectStatus::Activated) return false;
	if (!l_keys   || l_keys->m_ObjectStatus   != ObjectStatus::Activated) return false;
	if (!l_value  || l_value->m_ObjectStatus  != ObjectStatus::Activated) return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_viewport  = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_perFrameCB = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	// Graphics CL: m_Result is the only texture we own that needs a state
	// transition. PT's compute CL already left the noisy + hit buffers in
	// ReadOnly at the end of its dispatch; this pass's binding-layout
	// requested ReadOnly so the engine reads them through SRV views without
	// extra transition. The hash-grid UAV buffers stay in their persistent
	// ReadWrite state — same convention as LightCullingPass / LightPass.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_perFrameCB, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_noisy,      1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_hitPos,     2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_hitN,       3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_keys,       4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_value,      5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result,     6);

	// One thread per pixel, [numthreads(8,8,1)] — ceiling-divide so the
	// right and bottom strip on viewports that aren't multiples of 8
	// still cover. Same shape as PreTAA / GIDenoise dispatches. The
	// shader's own bounds check guards against threads past the viewport.
	const uint32_t l_groupsX = (uint32_t(l_viewport.x) + 7u) / 8u;
	const uint32_t l_groupsY = (uint32_t(l_viewport.y) + 7u) / 8u;
	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_groupsX, l_groupsY, 1);

	// Leave m_Result in WriteOnly; downstream LuminanceHistogramPass /
	// FinalBlendPass transition to ReadOnly via their own graphics-CL
	// reconciliation (matches TAA / PreTAA convention).
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

// TASK-182 — clear-on-bypass. Mirrors the live path's CL lifecycle
// (graphics CL transitions m_Result to WriteOnly, compute CL clears,
// both close) so the dispatch site's Execute / Signal pair drains
// uniformly. m_Result ends in WriteOnly, matching the live path.
bool GPUPathTracerDenoisePass::RecordClearCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	if (!m_Result || m_Result->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_textureService = g_Engine->Get<TextureResourceService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(m_Result, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_textureService->Clear(m_CommandListComp_Compute, m_Result);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	return true;
}

RenderPassComponent* GPUPathTracerDenoisePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* GPUPathTracerDenoisePass::GetResult()
{
	return m_Result;
}

void GPUPathTracerDenoisePass::CreateResult()
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_texService = g_Engine->Get<TextureResourceService>();

	m_Result = l_texService->Add("GPUPathTracerDenoisedBuffer");
	m_Result->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
	m_Result->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
	m_Result->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::RGBA;
	// Float32 to match GPUPathTracerPass::m_AccumulationBuffer — the noisy
	// input format dictates the denoised output format so LuminanceHistogram /
	// FinalBlend see the same precision regardless of which buffer feeds
	// l_hdrSource (PT-active vs rasterizer fork).
	m_Result->m_TextureDesc.PixelDataType    = TexturePixelDataType::Float32;
	m_Result->m_TextureDesc.Width            = l_resolution.x;
	m_Result->m_TextureDesc.Height           = l_resolution.y;
	m_Result->m_TextureDesc.DepthOrArraySize = 1;
	m_Result->m_CPUAccessibility             = Accessibility::Immutable;
	m_Result->m_GPUAccessibility             = Accessibility::ReadWrite;
	l_texService->Initialize(m_Result);
}

void GPUPathTracerDenoisePass::OnResize()
{
	auto l_texService = g_Engine->Get<TextureResourceService>();
	if (m_Result)
	{
		l_texService->Delete(m_Result);
		m_Result = nullptr;
	}
	CreateResult();
}
