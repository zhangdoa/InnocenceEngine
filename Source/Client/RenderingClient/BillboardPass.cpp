#include "BillboardPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/TemplateAssetService.h"

#include "OpaquePass.h"

#include "../../Engine/Engine.h"

using namespace Inno;


using namespace DefaultGPUBuffers;

bool BillboardPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_SamplerComp = l_renderingServer->AddSamplerComponent("BillboardPass/");

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("BillboardPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "billboardPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "billboardPass.frag/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("BillboardPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc = std::bind(&BillboardPass::DepthStencilRenderTargetsCreationFunc, this);
	l_RenderPassDesc.m_DepthStencilRenderTargetsReservationFunc = std::bind(&BillboardPass::DepthStencilRenderTargetsReservationFunc, this);

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(4);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

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

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_RenderPassComp->m_OnResize = std::bind(&BillboardPass::InitializeResourceBindingLayoutDescs, this);

	return true;
}

bool BillboardPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeSamplerComponent(m_SamplerComp);

    InitializeResourceBindingLayoutDescs();
	
	return true;
}

void BillboardPass::InitializeResourceBindingLayoutDescs()
{
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResource = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Vertex;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResource = m_SamplerComp;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Pixel;
}

bool BillboardPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BillboardPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BillboardPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_BillboardGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Billboard);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	auto l_mesh = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(ProceduralMeshShape::Square);

	auto& l_billboardPassDrawCallInfo = g_Engine->Get<RenderingContextService>()->GetBillboardPassDrawCallInfo();
	auto l_drawCallCount = l_billboardPassDrawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_iconTexture = l_billboardPassDrawCallInfo[i].iconTexture;
		auto l_offset = l_billboardPassDrawCallInfo[i].meshConstantBufferOffset;
		auto l_instanceCount = l_billboardPassDrawCallInfo[i].instanceCount;

		l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_BillboardGPUBufferComp, 1, l_offset, l_instanceCount);

		l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_iconTexture, 2);

		l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, l_mesh, l_instanceCount);

		l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_iconTexture, 2);
	}

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
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
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_RenderPassComp->m_DepthStencilRenderTarget.m_Texture = OpaquePass::Get().GetRenderPassComp()->m_DepthStencilRenderTarget.m_Texture;

    return true;
}