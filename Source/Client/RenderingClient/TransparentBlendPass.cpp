#include "TransparentBlendPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "TransparentGeometryProcessPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool TransparentBlendPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();
	
	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("TransparentBlendPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "transparentBlendPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("TransparentBlendPass/");

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 3;
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

bool TransparentBlendPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TransparentBlendPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TransparentBlendPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TransparentBlendPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	GPUResourceComponent* l_canvas = nullptr;

	if (renderingContext == nullptr)
	{
		l_renderingServer->ClearTextureComponent(m_RenderPassComp->m_RenderTargets[0].m_Texture);
		l_canvas = m_RenderPassComp->m_RenderTargets[0].m_Texture;
	}
	else
	{
		auto l_renderingContext = reinterpret_cast<TransparentBlendPassRenderingContext*>(renderingContext);
		l_canvas = l_renderingContext->m_output;
	}

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, TransparentGeometryProcessPass::Get().GetHeadPtrTexture(), 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, TransparentGeometryProcessPass::Get().GetResultChannel0(), 1);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, TransparentGeometryProcessPass::Get().GetResultChannel1(), 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_canvas, 3);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 4);

	l_renderingServer->Dispatch(m_RenderPassComp, 160, 90, 1);

	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, TransparentGeometryProcessPass::Get().GetHeadPtrTexture(), 0);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, TransparentGeometryProcessPass::Get().GetResultChannel0(), 1);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, TransparentGeometryProcessPass::Get().GetResultChannel1(), 2);
	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_canvas, 3);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* TransparentBlendPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* TransparentBlendPass::GetResult()
{
	return m_RenderPassComp->m_RenderTargets[0].m_Texture;
}