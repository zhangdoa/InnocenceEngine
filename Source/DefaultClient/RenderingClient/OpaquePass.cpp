#include "OpaquePass.h"
#include "OpaqueCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/DrawCallService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool OpaquePass::Setup(IServiceConfig *systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	m_ShaderProgramComp = l_graphicsService->AddShaderProgramComponent("OpaquePass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "opaqueGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "opaqueGeometryProcessPass.frag/";

	m_SamplerComp = l_graphicsService->AddSamplerComponent("OpaquePass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_RenderPassComp = l_graphicsService->AddRenderPassComponent("OpaquePass/");

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
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_SubresourceCount = 2; // Indirect root constants	

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

	// t0 - transform constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Vertex;
	
	// t1 - model constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ShaderStage = ShaderStage::Pixel;

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

	m_CommandListComp_Graphics = l_graphicsService->AddCommandListComponent("OpaquePass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;
	
	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool OpaquePass::Initialize()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Initialize(m_ShaderProgramComp);
	l_graphicsService->Initialize(m_RenderPassComp);
	l_graphicsService->Initialize(m_CommandListComp_Graphics);
	l_graphicsService->Initialize(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool OpaquePass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Delete(m_SamplerComp);	
	l_graphicsService->Delete(m_RenderPassComp);
	l_graphicsService->Delete(m_ShaderProgramComp);

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

	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_drawCallService = g_Engine->Get<DrawCallService>();

	l_graphicsService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_graphicsService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Graphics);
	l_graphicsService->ClearRenderTargets(m_RenderPassComp, m_CommandListComp_Graphics);

	auto l_perFrameCBuffer = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_perFrameCBufferPrev = g_Engine->Get<PerFrameDataService>()->GetPreviousFrameBuffer();
	auto l_transformCBuffer = l_drawCallService->GetCurrentFrameTransformBuffer();
	auto l_gpuModelDataCBuffer = l_drawCallService->GetGPUModelDataBuffer();
	auto l_materialCBuffer = l_drawCallService->GetMaterialBuffer();

	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex | ShaderStage::Pixel, l_perFrameCBuffer, 1);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex | ShaderStage::Pixel, l_perFrameCBufferPrev, 2);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex, l_transformCBuffer, 3);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, l_gpuModelDataCBuffer, 4);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, l_materialCBuffer, 5);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, nullptr, 6);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, m_SamplerComp, 7);

	auto l_indirectDrawCommandBuffer = reinterpret_cast<GPUBufferComponent*>(OpaqueCullingPass::Get().GetResult());
	l_graphicsService->ExecuteIndirect(m_RenderPassComp, m_CommandListComp_Graphics, l_indirectDrawCommandBuffer);
	
	l_graphicsService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

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

	auto l_graphicsService = g_Engine->getGraphicsService();	
	auto l_currentFrame = l_graphicsService->GetCurrentFrame();

	return m_RenderPassComp->m_OutputMergerTarget->m_ColorOutputs[2];
}