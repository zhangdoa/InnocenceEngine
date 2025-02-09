#include "BRDFLUTPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool BRDFLUTPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("BRDFLUTPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "BRDFLUTPass.comp/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("BRDFLUTPass/");
	m_Result = l_renderingServer->AddTextureComponent("BRDF LUT/");
	m_Result->m_TextureDesc.Width = 512;
	m_Result->m_TextureDesc.Height = 512;
	m_Result->m_TextureDesc.DepthOrArraySize = 1;
	m_Result->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_Result->m_TextureDesc.Usage = TextureUsage::ColorAttachment;
	m_Result->m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	m_Result->m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	m_Result->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;
	
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(1);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ColorAttachment;	
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool BRDFLUTPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_ShaderProgramComp);
	l_renderingServer->Initialize(m_RenderPassComp);
	l_renderingServer->Initialize(m_Result);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool BRDFLUTPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BRDFLUTPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BRDFLUTPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;
	
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
    l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Compute, m_Result, 0);
	l_renderingServer->Dispatch(m_RenderPassComp, 32, 32, 1);
	l_renderingServer->CommandListEnd(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;
	
	return true;
}

RenderPassComponent *BRDFLUTPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *BRDFLUTPass::GetResult()
{
	return m_Result;
}