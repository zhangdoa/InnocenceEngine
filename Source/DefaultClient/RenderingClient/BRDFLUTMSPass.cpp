#include "BRDFLUTMSPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"

#include "BRDFLUTPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/IGraphicsService.h"

using namespace Inno;

bool BRDFLUTMSPass::Setup(IServiceConfig *systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	m_ShaderProgramComp = l_graphicsService->AddShaderProgramComponent("BRDFLUTMSPass/");
	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "BRDFLUTMSPass.comp/";

	m_RenderPassComp = l_graphicsService->AddRenderPassComponent("BRDFLUTMSPass/");
	m_Result = l_graphicsService->AddTextureComponent("BRDF MS LUT/");
	m_Result->m_TextureDesc.Width = 512;
	m_Result->m_TextureDesc.Height = 512;
	m_Result->m_TextureDesc.DepthOrArraySize = 1;
	m_Result->m_TextureDesc.Sampler = TextureSampler::Sampler2D;
	m_Result->m_TextureDesc.Usage = TextureUsage::ComputeOnly;
	m_Result->m_TextureDesc.GPUAccessibility = Accessibility::ReadWrite;
	m_Result->m_TextureDesc.PixelDataType = TexturePixelDataType::Float16;
	m_Result->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;
	
	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(2);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_TextureUsage = TextureUsage::ComputeOnly;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ComputeOnly;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ShaderStage = ShaderStage::Compute;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Compute = l_graphicsService->AddCommandListComponent("BRDFLUTMSPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool BRDFLUTMSPass::Initialize()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Initialize(m_ShaderProgramComp);
	l_graphicsService->Initialize(m_RenderPassComp);
	l_graphicsService->Initialize(m_CommandListComp_Compute);
	l_graphicsService->Initialize(m_Result);

	m_ObjectStatus = ObjectStatus::Suspended;

	return true;
}

bool BRDFLUTMSPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Delete(m_Result);
	l_graphicsService->Delete(m_RenderPassComp);
	l_graphicsService->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus BRDFLUTMSPass::GetStatus()
{
	return m_ObjectStatus;
}

bool BRDFLUTMSPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;
			
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_graphicsService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);
    l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, BRDFLUTPass::Get().GetResult(), 0);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, m_Result, 1);
	l_graphicsService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, 32, 32, 1);
	l_graphicsService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;
	
	return true;
}

RenderPassComponent *BRDFLUTMSPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent *BRDFLUTMSPass::GetResult()
{
	return m_Result;
}