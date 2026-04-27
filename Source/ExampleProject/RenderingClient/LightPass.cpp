#include "LightPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/LightDataService.h"

#include "OpaquePass.h"
#include "BRDFLUTPass.h"
#include "BRDFLUTMSPass.h"
#include "SSAOPass.h"
#include "SunShadowRTPass.h"
#include "PointShadowGeometryProcessPass.h"
#include "LightCullingPass.h"
#include "GIFilterVerticalPass.h"
#include "VolumetricPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"

using namespace Inno;

bool LightPass::Setup(IServiceConfig *systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("LightPass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "lightPass.comp";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("LightPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&LightPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(23);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// b1 - PointLightCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	// b2 - SphereLightCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	// b4 - Voxelization CBuffer (HLSL register b3 unused after CSM removal;
	// keeping the register slot open avoids cascading renumbers across the
	// other lightPass.comp cbuffers).
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 4;

	// b5 - GI CBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 5;

	// t0 - World Position
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::ColorAttachment;

	// t1 - World Normal
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::ColorAttachment;

	// t2 - Albedo
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_TextureUsage = TextureUsage::ColorAttachment;

	// t3 - Metallic, Roughness, AO
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_TextureUsage = TextureUsage::ColorAttachment;

	// t4 - BRDF LUT
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_ShaderStage = ShaderStage::Compute;

	// t5 - BRDF LUT MS
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_TextureUsage = TextureUsage::ComputeOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ShaderStage = ShaderStage::Compute;

	// t6 - SSAO
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex = 6;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[11].m_TextureUsage = TextureUsage::ColorAttachment;

	// t8 - Light Culling Grid (HLSL register t7 unused after sun-CSM atlas
	// removal; keeping the register slot open avoids cascading renumbers).
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_DescriptorIndex = 8;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[12].m_TextureUsage = TextureUsage::ColorAttachment;

	// t9 - Light Index List
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[13].m_DescriptorIndex = 9;

	// t10 - Illuminance from Radiance Cache
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_DescriptorIndex = 10;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[14].m_TextureUsage = TextureUsage::ColorAttachment;

	// t11 - Volumetric Fog
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_DescriptorIndex = 11;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[15].m_TextureUsage = TextureUsage::ColorAttachment;

	// u0 - Luminance Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[16].m_TextureUsage = TextureUsage::ColorAttachment;

	// u1 - Illuminance Result
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[17].m_TextureUsage = TextureUsage::ColorAttachment;

	// s0 - Sampler linear
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[18].m_DescriptorIndex = 0;

	// s1 - Sampler point
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[19].m_DescriptorIndex = 1;

	// t12 - Point shadow atlas (TASK-148). Texture2DArray of packed linear
	// distance written by PointShadowGeometryProcessPass. The resolver
	// (shadowResolver.hlsl::PointShadowResolver) samples per active light's
	// atlasBaseSlot+face slice and applies the shadow term to tiled point
	// lighting in lightPassDirectLighting.hlsl::EvaluateTiledPointLighting.
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_DescriptorIndex = 12;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[20].m_TextureUsage = TextureUsage::ColorAttachment;

	// b6 - PointShadowCBuffer (TASK-148). Per-light cube-shadow metadata:
	// world-space light pos, range, atlas slot, isActive flag, view matrices.
	// The resolver reads lightPosWS_range and atlasBaseSlot; matrices are
	// caster-only.
	m_RenderPassComp->m_ResourceBindingLayoutDescs[21].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[21].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[21].m_DescriptorIndex = 6;

	// t13 - SunShadowRT visibility (TASK-138). Per-pixel R8 unorm produced by
	// SunShadowRTPass; sole sun-shadow input after the phase-2 swap.
	m_RenderPassComp->m_ResourceBindingLayoutDescs[22].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[22].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[22].m_DescriptorIndex = 13;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[22].m_TextureUsage = TextureUsage::ComputeOnly;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp_Linear = g_Engine->Get<SamplerResourceService>()->Add("LightPass/LinearSampler");
	m_SamplerComp_Point = g_Engine->Get<SamplerResourceService>()->Add("LightPass/PointSampler");
	m_SamplerComp_Point->m_SamplerDesc.m_MinFilterMethod = TextureFilterMethod::Nearest;
	m_SamplerComp_Point->m_SamplerDesc.m_MagFilterMethod = TextureFilterMethod::Nearest;

	m_CommandListComp_Compute = g_Engine->Get<CommandListResourceService>()->Add("LightPass/Compute");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("LightPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool LightPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Compute);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp_Linear);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp_Point);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool LightPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<TextureResourceService>()->Delete(m_LuminanceResult);
	g_Engine->Get<TextureResourceService>()->Delete(m_IlluminanceResult);

	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp_Point);
	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp_Linear);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus LightPass::GetStatus()
{
	return m_ObjectStatus;
}

bool LightPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	if (m_LuminanceResult->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "LuminanceResult not Activated, skipping.");
		return false;
	}

	if (m_IlluminanceResult->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "IlluminanceResult not Activated, skipping.");
		return false;
	}

	if (BRDFLUTPass::Get().GetResult()->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "BRDFLUTPass result not Activated, skipping.");
		return false;
	}

	if (BRDFLUTMSPass::Get().GetResult()->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "BRDFLUTMSPass result not Activated, skipping.");
		return false;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_currentFrame = l_fmService->GetCurrentFrame();

	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_PointLightGPUBufferComp = g_Engine->Get<LightDataService>()->GetPointLightBuffer();
	auto l_SphereLightGPUBufferComp = g_Engine->Get<LightDataService>()->GetSphereLightBuffer();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(BRDFLUTPass::Get().GetResult()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(BRDFLUTMSPass::Get().GetResult()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(SSAOPass::Get().GetResult()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	// SunShadowRT visibility transitions to ReadOnly here. Pass already left
	// it in ReadOnly at the end of its compute CL — this is a tracker
	// reconciliation no-op on the GPU side. Skipped if the RT pass is
	// suspended (e.g. TLAS not yet built post scene load).
	if (SunShadowRTPass::Get().GetStatus() == ObjectStatus::Activated)
		l_fmService->TryToTransitState(SunShadowRTPass::Get().GetResult(), m_CommandListComp_Graphics, Accessibility::ReadWrite, Accessibility::ReadOnly);
	// TASK-148: cube-shadow atlas — written by PointShadowGeometryProcessPass
	// as RTV (WriteOnly), consumed here as SRV (ReadOnly).
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(PointShadowGeometryProcessPass::Get().GetResult()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(reinterpret_cast<TextureComponent*>(LightCullingPass::Get().GetLightGrid()), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->TryToTransitState(GIFilterVerticalPass::Get().GetResult(), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
	l_fmService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Compute);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_SphereLightGPUBufferComp, 2);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[0], 5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[1], 6);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[2], 7);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[3], 8);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, BRDFLUTPass::Get().GetResult(), 9);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, BRDFLUTMSPass::Get().GetResult(), 10);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, SSAOPass::Get().GetResult(), 11);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 12);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 13);

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, GIFilterVerticalPass::Get().GetResult(), 14);
	// l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, VolumetricPass::GetRayMarchingResult(), 15);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_LuminanceResult, 16);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_IlluminanceResult, 17);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_SamplerComp_Linear, 18);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_SamplerComp_Point, 19);
	// TASK-148: cube-shadow atlas (t12) + per-light cbuffer (b6).
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, PointShadowGeometryProcessPass::Get().GetResult(), 20);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, g_Engine->Get<LightDataService>()->GetPointShadowBuffer(), 21);
	// TASK-138: SunShadowRT visibility (t13). Bound when available; nullptr
	// when the pass hasn't activated yet (e.g. early frames before TLAS
	// build) — engine binds a default zero descriptor on null, which causes
	// fully-shadowed sun until the RT pass activates.
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute,
	    SunShadowRTPass::Get().GetStatus() == ObjectStatus::Activated ? SunShadowRTPass::Get().GetResult() : nullptr, 22);

	// TASK-140 sample integration: wrap the dispatch in a paired GPU timer +
	// PIX event so PIX shows "LightPass" on the timeline and GetGpuTimings()
	// returns the per-frame ms cost. Pattern for further pass instrumentation
	// (TASK-138 RT sun shadows decision et al.) lives on GraphicsHardwareService.
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	l_hwService->BeginGpuPass(m_CommandListComp_Compute, "LightPass", GPUEngineType::Compute);

	l_fmService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_hwService->EndGpuPass(m_CommandListComp_Compute, "LightPass", GPUEngineType::Compute);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* LightPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

TextureComponent* LightPass::GetLuminanceResult()
{
	return m_LuminanceResult;
}

TextureComponent* LightPass::GetIlluminanceResult()
{
	return m_IlluminanceResult;
}

bool LightPass::RenderTargetsCreationFunc()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	if (m_LuminanceResult)
		g_Engine->Get<TextureResourceService>()->Delete(m_LuminanceResult);

	if (m_IlluminanceResult)
		g_Engine->Get<TextureResourceService>()->Delete(m_IlluminanceResult);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	m_LuminanceResult = g_Engine->Get<TextureResourceService>()->Add("LightPass Luminance Result");
	m_LuminanceResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_LuminanceResult->m_TextureDesc.Usage = TextureUsage::ComputeOnly;

	g_Engine->Get<TextureResourceService>()->Initialize(m_LuminanceResult);

	m_IlluminanceResult = g_Engine->Get<TextureResourceService>()->Add("LightPass Illuminance Result");
	m_IlluminanceResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_IlluminanceResult->m_TextureDesc.Usage = TextureUsage::ComputeOnly;

	g_Engine->Get<TextureResourceService>()->Initialize(m_IlluminanceResult);

	return true;
}