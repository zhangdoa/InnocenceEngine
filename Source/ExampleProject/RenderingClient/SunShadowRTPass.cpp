#include "SunShadowRTPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "OpaquePass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Component/TextureComponent.h"

using namespace Inno;

bool SunShadowRTPass::Setup(IServiceConfig* systemConfig)
{
	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SunShadowRTPass");

	// PSO subobject set: raygen + closest-hit + any-hit + primary-miss +
	// shadow-miss. The shadow path uses MissShaderIndex=1 (shadow-miss);
	// primary miss + closest-hit + any-hit are required by the PSO but
	// never executed (RAY_FLAG_FORCE_OPAQUE | ACCEPT_FIRST_HIT |
	// SKIP_CLOSEST_HIT) — see SunShadowRT*.hlsl headers.
	m_ShaderProgramComp->m_ShaderFilePaths.m_RayGenPath     = "SunShadowRTRayGen.hlsl";
	m_ShaderProgramComp->m_ShaderFilePaths.m_ClosestHitPath = "SunShadowRTClosestHit.hlsl";
	m_ShaderProgramComp->m_ShaderFilePaths.m_AnyHitPath     = "SunShadowRTAnyHit.hlsl";
	m_ShaderProgramComp->m_ShaderFilePaths.m_MissPath       = "SunShadowRTMiss.hlsl";
	m_ShaderProgramComp->m_ShaderFilePaths.m_ShadowMissPath = "SunShadowRTShadowMiss.hlsl";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SunShadowRTPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_GPUEngineType    = GPUEngineType::Compute;
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_UseRaytracing    = true;
	l_RenderPassDesc.m_UseOutputMerger  = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&SunShadowRTPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	// OnResize: visibility UAV is owned, not an output-merger target, so the
	// frame-management resize path skips it. Recreate at new resolution and
	// scrap any in-flight TAA accumulation history (resolution change forces
	// it anyway).
	m_RenderPassComp->m_OnResize = [this]() { OnResize(); };

	m_ShaderStage = ShaderStage::RayGen | ShaderStage::ClosestHit | ShaderStage::AnyHit | ShaderStage::Miss;

	// Binding layout (4 SRV/CBV + 1 UAV):
	// b0 — PerFrame CB
	// t0 — TLAS
	// t1 — opaque RT0 (world position + validity in .w)
	// t2 — opaque RT1 (world normal)
	// u0 — sun-visibility UAV
	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	// b0 — PerFrame CB (set 0, binding 0)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = m_ShaderStage;

	// t0 — TLAS (set 1, binding 0)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUBufferUsage = GPUBufferUsage::TLAS;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = m_ShaderStage;

	// t1 — opaque RT0 (world position + validity, set 1, binding 1)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = m_ShaderStage;

	// t2 — opaque RT1 (world normal, set 1, binding 2)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = m_ShaderStage;

	// u0 — sun-visibility UAV (set 2, binding 0)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = m_ShaderStage;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("SunShadowRTPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("SunShadowRTPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool SunShadowRTPass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);

	CreateVisibilityBuffer();

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SunShadowRTPass::Terminate()
{
	if (m_SunVisibility)
	{
		g_Engine->Get<TextureResourceService>()->Delete(m_SunVisibility);
		m_SunVisibility = nullptr;
	}

	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Delete(m_CommandListComp_Graphics);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowRTPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowRTPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "SunShadowRTPass: RenderPassComp not Activated, skipping.");
		return false;
	}

	if (!m_SunVisibility || m_SunVisibility->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "SunShadowRTPass: SunVisibility texture not Activated, skipping.");
		return false;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	// Graphics CL: cross-queue transitions for resources that may carry
	// PIXEL_SHADER_RESOURCE state from the swap chain or the rasterizer
	// pipeline. Required because compute command lists can't validly
	// transition out of pixel-shader-resource states.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0]), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::CrossQueueTransition);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1]), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::CrossQueueTransition);
	l_fmService->TryToTransitState(m_SunVisibility, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::ReadWrite);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	// Compute CL: bind resources + dispatch rays + leave the visibility UAV
	// in ReadOnly so LightPass's ReadOnly transition is a no-op.
	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, g_Engine->Get<GPUBufferResourceService>()->GetTLASBuffer(), 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, m_ShaderStage, m_SunVisibility, 4);

	// TASK-140 timer + PIX event. PIX timeline label "SunShadowRT"; cost
	// reported via GraphicsHardwareService::GetGpuTimings(). The whole
	// point of TASK-138 phase 1 — quote this number in the closure
	// alongside the existing CSM+PCSS time.
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	l_hwService->BeginGpuPass(m_CommandListComp_Compute, "SunShadowRT", GPUEngineType::Compute);

	l_fmService->DispatchRays(m_RenderPassComp, m_CommandListComp_Compute, l_resolution.x, l_resolution.y, 1);

	l_hwService->EndGpuPass(m_CommandListComp_Compute, "SunShadowRT", GPUEngineType::Compute);

	l_fmService->TryToTransitState(m_SunVisibility, m_CommandListComp_Compute, Accessibility::ReadWrite, Accessibility::ReadOnly);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SunShadowRTPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* SunShadowRTPass::GetResult()
{
	return m_SunVisibility;
}

bool SunShadowRTPass::RenderTargetsCreationFunc()
{
	// Owned visibility UAV; created in Initialize/OnResize. Nothing for the
	// engine's RenderTargetsInitializationFunc to do — the pass has no
	// output-merger render targets.
	return true;
}

void SunShadowRTPass::CreateVisibilityBuffer()
{
	auto l_resolution = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_texService = g_Engine->Get<TextureResourceService>();

	m_SunVisibility = l_texService->Add("SunShadowRT_Visibility");
	m_SunVisibility->m_TextureDesc.Sampler          = TextureSampler::Sampler2D;
	m_SunVisibility->m_TextureDesc.Usage            = TextureUsage::ComputeOnly;
	m_SunVisibility->m_TextureDesc.PixelDataFormat  = TexturePixelDataFormat::R;
	// R8 unorm is enough — visibility is in [0,1] and a single jittered
	// sample per frame quantises to ~256 levels. TAA accumulation in the
	// downstream consumer (LightPass + post-TAA) smooths the penumbra.
	m_SunVisibility->m_TextureDesc.PixelDataType    = TexturePixelDataType::UByte;
	m_SunVisibility->m_TextureDesc.Width            = l_resolution.x;
	m_SunVisibility->m_TextureDesc.Height           = l_resolution.y;
	m_SunVisibility->m_TextureDesc.DepthOrArraySize = 1;
	m_SunVisibility->m_CPUAccessibility             = Accessibility::Immutable;
	m_SunVisibility->m_GPUAccessibility             = Accessibility::ReadWrite;
	l_texService->Initialize(m_SunVisibility);
}

void SunShadowRTPass::OnResize()
{
	auto l_texService = g_Engine->Get<TextureResourceService>();
	if (m_SunVisibility)
	{
		l_texService->Delete(m_SunVisibility);
		m_SunVisibility = nullptr;
	}
	CreateVisibilityBuffer();
}
