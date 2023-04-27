#include "TAAPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool TAAPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("TAAPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "TAAPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("TAAPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 2;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool TAAPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TAAPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TAAPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TAAPass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_renderingContext = reinterpret_cast<TAAPassRenderingContext*>(renderingContext);	
	auto l_viewportSize = g_Engine->getRenderingFrontend()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp, (size_t)(l_renderingContext->m_writeTexture == m_RenderPassComp->m_RenderTargets[0].m_Texture));

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_readTexture, 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_motionVector, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_writeTexture, 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 4);

	l_renderingServer->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_readTexture, 1);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_motionVector, 2);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_writeTexture, 3);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* TAAPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *TAAPass::GetOddResult()
{
	return m_RenderPassComp->m_RenderTargets[0].m_Texture;
}

GPUResourceComponent *TAAPass::GetEvenResult()
{
	return m_RenderPassComp->m_RenderTargets[1].m_Texture;
}
