#include "SunShadowGeometryProcessPass.h"
#include "SunShadowCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Common/Timer.h"

using namespace Inno;

bool SunShadowGeometryProcessPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_shadowMapResolution = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().shadowMapResolution;

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("SunShadowGeometryProcessPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "sunShadowGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_GSPath = "sunShadowGeometryProcessPass.geom/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "sunShadowGeometryProcessPass.frag/";

	m_SamplerComp = l_renderingServer->AddSamplerComponent("SunShadowGeometryProcessPass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("SunShadowGeometryProcessPass/");


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

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowGeometryProcessPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SunShadowGeometryProcessPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_SamplerComp);	
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

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
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);

	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	auto l_renderingContextService = g_Engine->Get<RenderingContextService>();
	auto l_transformCBuffer = l_renderingContextService->GetGPUBufferComponent(GPUBufferUsageType::Transform);
	auto l_gpuModelDataBuffer = l_renderingContextService->GetGPUBufferComponent(GPUBufferUsageType::GPUModelData);
	auto l_CSMCBuffer = l_renderingContextService->GetGPUBufferComponent(GPUBufferUsageType::CSM);
	auto l_materialCBuffer = l_renderingContextService->GetGPUBufferComponent(GPUBufferUsageType::Material);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_transformCBuffer, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Geometry, l_CSMCBuffer, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_gpuModelDataBuffer, 3);	
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_materialCBuffer, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, nullptr, 5);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 6);

	// Use the indirect draw command buffer from SunShadowCullingPass
	auto l_indirectDrawCommandBuffer = reinterpret_cast<GPUBufferComponent*>(SunShadowCullingPass::Get().GetResult());
	l_renderingServer->ExecuteIndirect(m_RenderPassComp, l_indirectDrawCommandBuffer);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

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
		return nullptr;
	
	if (!m_RenderPassComp->m_OutputMergerTarget)
		return nullptr;

	auto l_renderingServer = g_Engine->getRenderingServer();	
	auto l_currentFrame = l_renderingServer->GetCurrentFrame();

	return m_RenderPassComp->m_OutputMergerTarget->m_ColorOutputs[0];
}

uint32_t SunShadowGeometryProcessPass::GetShadowMapResolution()
{
	return m_shadowMapResolution;
}