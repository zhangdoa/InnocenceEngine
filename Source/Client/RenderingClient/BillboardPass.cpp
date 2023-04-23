#include "BillboardPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool BillboardPass::Setup(ISystemConfig *systemConfig)
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("BillboardPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "billboardPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "billboardPass.frag/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("BillboardPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc = std::bind(&BillboardPass::DepthStencilRenderTargetsCreationFunc, this);

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

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("BillboardPass/");

	return true;
}

bool BillboardPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);

	return true;
}

bool BillboardPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BillboardPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BillboardPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_BillboardGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Billboard);

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 3);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);

	auto l_mesh = g_Engine->getRenderingFrontend()->GetMeshComponent(ProceduralMeshShape::Square);

	auto& l_billboardPassDrawCallInfo = g_Engine->getRenderingFrontend()->GetBillboardPassDrawCallInfo();
	auto l_drawCallCount = l_billboardPassDrawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_iconTexture = l_billboardPassDrawCallInfo[i].iconTexture;
		auto l_offset = l_billboardPassDrawCallInfo[i].meshConstantBufferOffset;
		auto l_instanceCount = l_billboardPassDrawCallInfo[i].instanceCount;

		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_BillboardGPUBufferComp, 1, Accessibility::ReadOnly, l_offset, l_instanceCount);

		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_iconTexture, 2);

		g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp, l_mesh, l_instanceCount);

		g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_iconTexture, 2);
	}

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* BillboardPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool Inno::BillboardPass::DepthStencilRenderTargetsCreationFunc()
{
	m_RenderPassComp->m_DepthStencilRenderTarget = OpaquePass::Get().GetRenderPassComp()->m_DepthStencilRenderTarget;

    return true;
}