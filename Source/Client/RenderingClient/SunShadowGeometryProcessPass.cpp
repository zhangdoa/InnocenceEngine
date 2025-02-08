#include "SunShadowGeometryProcessPass.h"

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

	m_IndirectDrawCommand = l_renderingServer->AddGPUBufferComponent("SunShadowGeometryProcessPass/IndirectDrawCommand/");
	m_IndirectDrawCommand->m_Usage = GPUBufferUsage::IndirectDraw;

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

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(6);

	// b0 - Object Index
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IsRootConstant = true;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// b1 - PerObjectCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Vertex | ShaderStage::Pixel;

	// b2 - CSM
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Geometry;

	// t0 - Material
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Pixel;

	// t1 - Textures
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::Sample;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Pixel;

	// s0 - Sampler
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Pixel;

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
	l_renderingServer->Initialize(m_IndirectDrawCommand);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool SunShadowGeometryProcessPass::Update()
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_IndirectDrawCommand->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto& l_drawCallList = g_Engine->Get<RenderingContextService>()->GetDrawCallInfo();
	auto f_drawCallValid = [](const DrawCallInfo& drawCall) 
	{ 
		auto l_visible = static_cast<uint32_t>(drawCall.m_VisibilityMask & VisibilityMask::Sun);
		return l_visible && drawCall.meshUsage != MeshUsage::Skeletal;
	};

	l_renderingServer->UpdateIndirectDrawCommand(m_IndirectDrawCommand, l_drawCallList, f_drawCallValid);

	return true;
}

bool SunShadowGeometryProcessPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_RenderPassComp);

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

	auto l_perObjectCBuffer = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_CSMCBuffer = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::CSM);
	auto l_materialCBuffer = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Material);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_perObjectCBuffer, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_CSMCBuffer, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_materialCBuffer, 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, nullptr, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_SamplerComp, 5);

	l_renderingServer->ExecuteIndirect(m_RenderPassComp, m_IndirectDrawCommand);

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