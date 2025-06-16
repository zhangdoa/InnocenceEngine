#include "OpaquePass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool OpaquePass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("OpaquePass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "opaqueGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "opaqueGeometryProcessPass.frag/";

	m_SamplerComp = l_renderingServer->AddSamplerComponent("OpaquePass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("OpaquePass/");

	m_IndirectDrawCommand = l_renderingServer->AddGPUBufferComponent("OpaquePass/IndirectDrawCommand/");
	m_IndirectDrawCommand->m_Usage = GPUBufferUsage::IndirectDraw;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 4;
	l_RenderPassDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_IndirectDraw = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(8);

	// b0 - root constant - object index
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IsRootConstant = true;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// b1 - per frame constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Vertex;

	// b2 - previous frame per frame constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Pixel;

	// t0 - per object constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Vertex | ShaderStage::Pixel;
	
	// t1 - previous frame per object constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Vertex | ShaderStage::Pixel;

	// t2 - material constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_ShaderStage = ShaderStage::Pixel;

	// t3 - textures
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_TextureUsage = TextureUsage::Sample;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_ShaderStage = ShaderStage::Pixel;

	// s0 - sampler
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_ShaderStage = ShaderStage::Pixel;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;
	
	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool OpaquePass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_SamplerComp);
	l_renderingServer->Initialize(m_IndirectDrawCommand);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool OpaquePass::Update()
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_IndirectDrawCommand->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	auto& l_drawCallList = g_Engine->Get<RenderingContextService>()->GetDrawCallInfo();
	auto f_drawCallValid = [](const DrawCallInfo& drawCall) 
	{ 
		auto l_visible = static_cast<uint32_t>(drawCall.m_VisibilityMask & VisibilityMask::MainCamera);
		return l_visible && drawCall.meshUsage != MeshUsage::Skeletal;
	};

	l_renderingServer->UpdateIndirectDrawCommand(m_IndirectDrawCommand, l_drawCallList, f_drawCallValid);

	return true;
}

bool OpaquePass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_IndirectDrawCommand);
	l_renderingServer->Delete(m_SamplerComp);	
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus OpaquePass::GetStatus()
{
	return m_ObjectStatus;
}

bool OpaquePass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	auto l_perFrameCBuffer = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_perObjectCBuffer = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_materialCBuffer = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::Material);
	auto l_perFrameCBufferPrev = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFramePrev);
	auto l_perObjectCBufferPrev = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::MeshPrev);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_perFrameCBuffer, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_perFrameCBufferPrev, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex | ShaderStage::Pixel, l_perObjectCBuffer, 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_perObjectCBufferPrev, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_materialCBuffer, 5);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, nullptr, 6);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 7);

	l_renderingServer->ExecuteIndirect(m_RenderPassComp, m_IndirectDrawCommand);
	
	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;
	
	return true;
}

RenderPassComponent* OpaquePass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* OpaquePass::GetResult()
{
	if (!m_RenderPassComp)
		return nullptr;
	
	if (!m_RenderPassComp->m_OutputMergerTarget)
		return nullptr;

	auto l_renderingServer = g_Engine->getRenderingServer();	
	auto l_currentFrame = l_renderingServer->GetCurrentFrame();

	return m_RenderPassComp->m_OutputMergerTarget->m_ColorOutputs[2];
}