#include "BillboardPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace BillboardPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool BillboardPass::Setup()
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("BillboardPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "billboardPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "billboardPass.frag/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("BillboardPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(4);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 12;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Sampler;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("BillboardPass/");

	return true;
}

bool BillboardPass::Initialize()
{
	m_RPDC->m_DepthStencilRenderTarget = OpaquePass::GetRPDC()->m_DepthStencilRenderTarget;

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool BillboardPass::Render()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_BillboardGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Billboard);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_SDC, 3);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);

	auto l_mesh = g_Engine->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Square);

	auto& l_billboardPassDrawCallInfo = g_Engine->getRenderingFrontend()->getBillboardPassDrawCallInfo();
	auto l_drawCallCount = l_billboardPassDrawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_iconTexture = l_billboardPassDrawCallInfo[i].iconTexture;
		auto l_offset = l_billboardPassDrawCallInfo[i].meshConstantBufferOffset;
		auto l_instanceCount = l_billboardPassDrawCallInfo[i].instanceCount;

		g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_BillboardGBDC, 1, Accessibility::ReadOnly, l_offset, l_instanceCount);

		g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_iconTexture, 2);

		g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RPDC, l_mesh, l_instanceCount);

		g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, l_iconTexture, 2);
	}

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool BillboardPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent* BillboardPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* BillboardPass::GetSPC()
{
	return m_SPC;
}