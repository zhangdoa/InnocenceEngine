#include "BillboardPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/BillboardDrawCallService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "OpaquePass.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool BillboardPass::Setup(IServiceConfig* systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	m_SamplerComp = l_graphicsService->AddSamplerComponent("BillboardPass/");

	m_ShaderProgramComp = l_graphicsService->AddShaderProgramComponent("BillboardPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "billboardPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "billboardPass.frag/";

	m_RenderPassComp = l_graphicsService->AddRenderPassComponent("BillboardPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_DepthStencilRenderTargetsInitializationFunc = std::bind(&BillboardPass::DepthStencilRenderTargetsCreationFunc, this);
	l_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc = std::bind(&BillboardPass::DepthStencilRenderTargetsReservationFunc, this);

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(4);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Vertex;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 12;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Pixel;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = l_graphicsService->AddCommandListComponent("BillboardPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool BillboardPass::Initialize()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Initialize(m_ShaderProgramComp);
	l_graphicsService->Initialize(m_RenderPassComp);
	l_graphicsService->Initialize(m_SamplerComp);
	l_graphicsService->Initialize(m_CommandListComp_Graphics);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool BillboardPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Delete(m_SamplerComp);
	l_graphicsService->Delete(m_RenderPassComp);
	l_graphicsService->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BillboardPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BillboardPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_BillboardGPUBufferComp = g_Engine->Get<BillboardDrawCallService>()->GetBillboardBuffer();

	// @TODO: Use indirect draw command

	return false;
}

RenderPassComponent* BillboardPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool BillboardPass::DepthStencilRenderTargetsReservationFunc()
{
	return true;
}

bool BillboardPass::DepthStencilRenderTargetsCreationFunc()
{
	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	l_outputMergerTarget->m_DepthStencilOutput = OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_DepthStencilOutput;

	return true;
}