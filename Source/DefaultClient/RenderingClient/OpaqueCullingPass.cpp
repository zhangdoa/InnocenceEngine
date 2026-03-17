#include "OpaqueCullingPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool OpaqueCullingPass::Setup(ISystemConfig* systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("OpaqueCullingPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "opaqueGPUCulling.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("OpaqueCullingPass/");

	m_IndirectDrawCommandBuffer = l_renderingServer->AddGPUBufferComponent("OpaqueCullingPass/IndirectDrawCommandBuffer/");
	m_IndirectDrawCommandBuffer->m_Usage = GPUBufferUsage::IndirectDraw;
	m_IndirectDrawCommandBuffer->m_ElementCount = 512;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(4);

	// b0 - PerFrame constant buffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	// t0 - GPU model data buffer (input - StructuredBuffer)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	// t1 - Material buffer (input - StructuredBuffer)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Compute;

	// u0 - Indirect draw command buffer (output - RWStructuredBuffer)
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = l_renderingServer->AddCommandListComponent("OpaqueCullingPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool OpaqueCullingPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_CommandListComp_Compute);
	l_renderingServer->Initialize(m_IndirectDrawCommandBuffer);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool OpaqueCullingPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_IndirectDrawCommandBuffer);
	l_renderingServer->Delete(m_RenderPassComp);
	l_renderingServer->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus OpaqueCullingPass::GetStatus()
{
	return m_ObjectStatus;
}

bool OpaqueCullingPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_IndirectDrawCommandBuffer->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	auto l_renderingContextService = g_Engine->Get<RenderingContextService>();
	auto& l_gpuModelData = l_renderingContextService->GetGPUModelData();
	uint32_t l_modelCount = static_cast<uint32_t>(l_gpuModelData.size());
	if (l_modelCount == 0)
		return false;
	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	// Bind resources for compute shader
	auto l_perFrameCBuffer = l_renderingContextService->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_gpuModelDataBuffer = l_renderingContextService->GetGPUBufferComponent(GPUBufferUsageType::GPUModelData);
	auto l_materialBuffer = l_renderingContextService->GetGPUBufferComponent(GPUBufferUsageType::Material);

	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_perFrameCBuffer, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_gpuModelDataBuffer, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_materialBuffer, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_IndirectDrawCommandBuffer, 3);

	// Dispatch culling compute shader
	// Calculate thread groups based on model count
	uint32_t l_threadGroupSize = 64; // Typical compute shader thread group size
	uint32_t l_threadGroups = (l_modelCount + l_threadGroupSize - 1) / l_threadGroupSize;

	l_renderingServer->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, l_threadGroups, 1, 1);

	l_renderingServer->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

RenderPassComponent* OpaqueCullingPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* OpaqueCullingPass::GetResult()
{
	return m_IndirectDrawCommandBuffer;
}