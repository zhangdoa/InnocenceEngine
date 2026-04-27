#include "PointShadowGeometryProcessPass.h"
#include "ShadowCasterCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/LightDataService.h"
#include "../../Engine/Services/DrawCallService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Component/TextureComponent.h"

using namespace Inno;

bool PointShadowGeometryProcessPass::Setup(IServiceConfig* /*systemConfig*/)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("PointShadowGeometryProcessPass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "pointShadowGeometryProcessPass.vert";
	m_ShaderProgramComp->m_ShaderFilePaths.m_GSPath = "pointShadowGeometryProcessPass.geom";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "pointShadowGeometryProcessPass.frag";

	m_SamplerComp = g_Engine->Get<SamplerResourceService>()->Add("PointShadowGeometryProcessPass");
	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("PointShadowGeometryProcessPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_Resizable = false;
	l_RenderPassDesc.m_IndirectDraw = true;

	// The atlas is owned by LightDataService (TASK-147). Two hooks are needed:
	//   - reservation: bind m_OutputMergerTarget->m_ColorOutputs[0] to the
	//     externally-owned atlas (same idiom as AnimationPass aliasing
	//     OpaquePass's RTs). Without this, the default reservation path would
	//     Add() a fresh TextureComponent and downstream code would never see
	//     the atlas LightDataService allocated.
	//   - initialization: no-op for the color RT (the atlas is already
	//     initialized by LightDataService::Initialize). Without overriding,
	//     the default initialization stamps m_RenderTargetDesc onto the
	//     existing TextureComponent and re-runs InitializeSynchronous, which
	//     would clobber the LightDataService-owned resource.
	// The depth-stencil target is auto-created (no DS hooks set) at the same
	// array shape (Sampler2DArray, 256² × 48 slices) — Width/Height/Sampler/
	// DepthOrArraySize come from m_RenderTargetDesc above; the default DS
	// initializer overrides Usage→DepthAttachment, Format→Depth.
	l_RenderPassDesc.m_RenderTargetsCreationFunc       = std::bind(&PointShadowGeometryProcessPass::RenderTargetsReservationFunc, this);
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = []() { return true; };

	auto l_RenderingCapability = g_Engine->Get<RenderingConfigurationService>()->GetRenderingCapability();
	const uint32_t l_AtlasSliceCount = l_RenderingCapability.maxPointShadows * 6;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler           = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width             = m_PerFaceResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height            = m_PerFaceResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize  = l_AtlasSliceCount;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat   = TexturePixelDataFormat::RG;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType     = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 0.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;
	// Caster outputs (linearDist, linearDist², 0, 1) at the far plane on
	// unrendered texels — clear matches so PCSS doesn't see false blockers.
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[2] = 0.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[3] = 1.0f;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width  = (float)m_PerFaceResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_PerFaceResolution;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	// Render both faces of every triangle so thin / single-sided occluders
	// write depth. Adaptive depth bias in shadowResolver.hlsl handles
	// Peter-Panning.
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(7);

	// b0 - Object Index (root constant — indirect-draw signature mirrors sun)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IsRootConstant = true;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_SubresourceCount = 2; // Indirect root constants

	// t0 - Transform
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Vertex | ShaderStage::Pixel;

	// b1 - PointShadowCBuffer (consumed by GS for per-light × per-face fan-out
	// and by PS to recover lightPosWS / range for linear-distance write).
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Geometry | ShaderStage::Pixel;

	// t1 - Model Data
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Pixel;

	// t2 - Material
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Pixel;

	// t3 - Textures (bindless albedo array for alpha-test)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_TextureUsage = TextureUsage::Sample;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Pixel;

	// s0 - Sampler
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage = ShaderStage::Pixel;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("PointShadowGeometryProcessPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PointShadowGeometryProcessPass::Initialize()
{
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Suspended;
	return true;
}

bool PointShadowGeometryProcessPass::Terminate()
{
	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus PointShadowGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool PointShadowGeometryProcessPass::PrepareCommandList(IRenderingContext* /*renderingContext*/)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "PointShadowGeometryProcessPass: RenderPassComp not Activated, skipping.");
		return false;
	}

	auto l_lightDataService = g_Engine->Get<LightDataService>();
	const uint32_t l_ShadowCount = l_lightDataService->GetPointShadowCount();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Graphics);
	l_fmService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Graphics);

	// No active shadow casters — clear has produced an all-far-plane atlas;
	// the resolver short-circuits per atlas-slot when the cbuffer's isActive
	// flag is 0, so the cleared atlas is harmless. Skip the draw work.
	if (l_ShadowCount == 0)
	{
		l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);
		m_ObjectStatus = ObjectStatus::Activated;
		return true;
	}

	auto l_drawCallService = g_Engine->Get<DrawCallService>();
	auto l_transformCBuffer        = l_drawCallService->GetCurrentFrameTransformBuffer();
	auto l_gpuModelDataBuffer      = l_drawCallService->GetGPUModelDataBuffer();
	auto l_pointShadowCBuffer      = l_lightDataService->GetPointShadowBuffer();
	auto l_materialCBuffer         = l_drawCallService->GetMaterialBuffer();

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex,   l_transformCBuffer,   1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Geometry, l_pointShadowCBuffer, 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel,    l_gpuModelDataBuffer, 3);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel,    l_materialCBuffer,    4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel,    nullptr,              5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel,    m_SamplerComp,        6);

	// Shares the shadow-caster indirect draw command buffer with other shadow
	// passes — point shadow casts the same opaque scene with a different
	// projection. Per-light frustum culling could roughly halve draw count
	// (follow-up optimization); current trade is a small extra GS bandwidth
	// cost for a shorter integration path.
	auto l_indirectDrawCommandBuffer = reinterpret_cast<GPUBufferComponent*>(ShadowCasterCullingPass::Get().GetResult());
	l_fmService->ExecuteIndirect(m_RenderPassComp, m_CommandListComp_Graphics, l_indirectDrawCommandBuffer);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	m_ObjectStatus = ObjectStatus::Activated;
	return true;
}

RenderPassComponent* PointShadowGeometryProcessPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

uint32_t PointShadowGeometryProcessPass::GetShadowMapResolution()
{
	return m_PerFaceResolution;
}

GPUResourceComponent* PointShadowGeometryProcessPass::GetResult()
{
	// Atlas lifetime is owned by LightDataService — return its handle so
	// LightPass binds the same resource the caster wrote.
	return g_Engine->Get<LightDataService>()->GetPointShadowAtlas();
}

bool PointShadowGeometryProcessPass::RenderTargetsReservationFunc()
{
	if (m_RenderPassComp->m_OutputMergerTarget == nullptr)
		g_Engine->Get<RenderPassResourceService>()->Add(m_RenderPassComp->m_OutputMergerTarget);

	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	l_outputMergerTarget->m_ColorOutputs.resize(m_RenderPassComp->m_RenderPassDesc.m_RenderTargetCount);
	l_outputMergerTarget->m_ColorOutputs[0] = g_Engine->Get<LightDataService>()->GetPointShadowAtlas();

	return true;
}
