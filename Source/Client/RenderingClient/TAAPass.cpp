#include "TAAPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool TAAPass::Setup(ISystemConfig *systemConfig)
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("TAAPass/");

	m_SPC->m_ShaderFilePaths.m_CSPath = "TAAPass.comp/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("TAAPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;

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

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_oddTextureComp = g_Engine->getRenderingServer()->AddTextureComponent("TAAPass_Odd/");
	m_evenTextureComp = g_Engine->getRenderingServer()->AddTextureComponent("TAAPass_Even/");

	m_oddTextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_evenTextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool TAAPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_oddTextureComp);
	g_Engine->getRenderingServer()->InitializeTextureComponent(m_evenTextureComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TAAPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TAAPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TAAPass::PrepareCommandList(IRenderingContext* renderingContext)
{	
	auto l_renderingContext = reinterpret_cast<TAAPassRenderingContext*>(renderingContext);	
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	TextureComponent *l_writeTextureComp;
	TextureComponent *l_ReadTextureComp;

	if (m_isPassOdd)
	{
		l_ReadTextureComp = m_oddTextureComp;
		l_writeTextureComp = m_evenTextureComp;

		m_isPassOdd = false;
	}
	else
	{
		l_ReadTextureComp = m_evenTextureComp;
		l_writeTextureComp = m_oddTextureComp;

		m_isPassOdd = true;
	}

	g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_ReadTextureComp, 1);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3], 2);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_writeTextureComp, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 4);

	g_Engine->getRenderingServer()->Dispatch(m_RenderPassComp, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_renderingContext->m_input, 0);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_ReadTextureComp, 1);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[3], 2);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Compute, l_writeTextureComp, 3, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* TAAPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *TAAPass::GetResult()
{
	if (m_isPassOdd)
	{
		return m_evenTextureComp;
	}
	else
	{
		return m_oddTextureComp;
	}
}
