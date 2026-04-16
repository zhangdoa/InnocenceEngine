#include "SunShadowGeometryProcessPass.h"
#include "SunShadowCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/LightDataService.h"
#include "../../Engine/Services/DrawCallService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"
#include "../../Engine/Common/Timer.h"

using namespace Inno;

bool SunShadowGeometryProcessPass::Setup(IServiceConfig *systemConfig)
{	
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_shadowMapResolution = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().shadowMapResolution;

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("SunShadowGeometryProcessPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "sunShadowGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_GSPath = "sunShadowGeometryProcessPass.geom/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "sunShadowGeometryProcessPass.frag/";

	m_SamplerComp = g_Engine->Get<SamplerResourceService>()->Add("SunShadowGeometryProcessPass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("SunShadowGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_Resizable = false;
	l_RenderPassDesc.m_IndirectDraw = true;
	
	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = m_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = m_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;
	// Must match sunShadowGeometryProcessPass.frag output: float4(depth, depth*depth, 0, 1)
	// at far plane (depth=1). PCSS reads .r as blocker depth; clearing to 1.0 prevents
	// unrendered texels from registering as blockers at depth 0.
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[2] = 0.0f;
	l_RenderPassDesc.m_RenderTargetDesc.ClearColor[3] = 1.0f;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_shadowMapResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_shadowMapResolution;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerCullMode = RasterizerCullMode::Front;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(7);

	// b0 - Object Index
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

	// b1 - CSM
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Geometry;
	
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

	// t3 - Textures
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

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("SunShadowGeometryProcessPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowGeometryProcessPass::Initialize()
{	
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	
	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SunShadowGeometryProcessPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp);	
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
	{
		Log(Warning, "RenderPassComp not Activated, skipping.");
		return false;
	}

	auto l_fmService = g_Engine->Get<FrameManagementService>();

	l_fmService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Graphics);

	l_fmService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Graphics);

	auto l_drawCallService = g_Engine->Get<DrawCallService>();
	auto l_transformCBuffer = l_drawCallService->GetCurrentFrameTransformBuffer();
	auto l_gpuModelDataBuffer = l_drawCallService->GetGPUModelDataBuffer();
	auto l_CSMCBuffer = g_Engine->Get<LightDataService>()->GetCSMBuffer();
	auto l_materialCBuffer = l_drawCallService->GetMaterialBuffer();

	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex, l_transformCBuffer, 1);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Geometry, l_CSMCBuffer, 2);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, l_gpuModelDataBuffer, 3);	
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, l_materialCBuffer, 4);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, nullptr, 5);
	l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, m_SamplerComp, 6);

	// Use the indirect draw command buffer from SunShadowCullingPass
	auto l_indirectDrawCommandBuffer = reinterpret_cast<GPUBufferComponent*>(SunShadowCullingPass::Get().GetResult());
	l_fmService->ExecuteIndirect(m_RenderPassComp, m_CommandListComp_Graphics, l_indirectDrawCommandBuffer);

	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* SunShadowGeometryProcessPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SunShadowGeometryProcessPass::GetResult()
{
	if (!m_RenderPassComp)
		return false;
	
	if (!m_RenderPassComp->m_OutputMergerTarget)
		return false;

	auto l_fmService = g_Engine->Get<FrameManagementService>();
	auto l_currentFrame = l_fmService->GetCurrentFrame();

	return m_RenderPassComp->m_OutputMergerTarget->m_ColorOutputs[0];
}

uint32_t SunShadowGeometryProcessPass::GetShadowMapResolution()
{
	return m_shadowMapResolution;
}