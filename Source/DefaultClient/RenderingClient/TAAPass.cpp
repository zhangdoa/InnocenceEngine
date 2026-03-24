#include "TAAPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"

#include "../../Engine/Engine.h"

using namespace Inno;

bool TAAPass::Setup(IServiceConfig* systemConfig)
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	m_ShaderProgramComp = l_graphicsService->AddShaderProgramComponent("TAAPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_CSPath = "TAAPass.comp/";

	m_RenderPassComp = l_graphicsService->AddRenderPassComponent("TAAPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&TAAPass::RenderTargetsCreationFunc, this);

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);

	// b0 - PerFrameCBuffer
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	// t0 - Input
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_TextureUsage = TextureUsage::ColorAttachment;

	// t1 - Read Texture
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_TextureUsage = TextureUsage::ColorAttachment;

	// t2 - Motion Vector Image
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_TextureUsage = TextureUsage::ColorAttachment;

	// u0 - Write Texture
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_TextureUsage = TextureUsage::ColorAttachment;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_CommandListComp_Graphics = l_graphicsService->AddCommandListComponent("TAAPass/Graphics/");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_CommandListComp_Compute = l_graphicsService->AddCommandListComponent("TAAPass/Compute/");
	m_CommandListComp_Compute->m_Type = GPUEngineType::Compute;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool TAAPass::Initialize()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Initialize(m_ShaderProgramComp);
	l_graphicsService->Initialize(m_RenderPassComp);
	l_graphicsService->Initialize(m_CommandListComp_Graphics);
	l_graphicsService->Initialize(m_CommandListComp_Compute);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool TAAPass::Terminate()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	l_graphicsService->Delete(m_OddTextureComp);
	l_graphicsService->Delete(m_EvenTextureComp);
	l_graphicsService->Delete(m_CommandListComp_Compute);
	l_graphicsService->Delete(m_CommandListComp_Graphics);
	l_graphicsService->Delete(m_RenderPassComp);
	l_graphicsService->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus TAAPass::GetStatus()
{
	return m_ObjectStatus;
}

bool TAAPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	if (m_RenderPassComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;

	if (m_EvenTextureComp->m_ObjectStatus != ObjectStatus::Activated
		|| m_OddTextureComp->m_ObjectStatus != ObjectStatus::Activated)
		return false;
			
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_frameCount = l_graphicsService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;
	auto l_readTexture = l_isOddFrame ? m_EvenTextureComp : m_OddTextureComp;
	auto l_writeTexture = l_isOddFrame ? m_OddTextureComp : m_EvenTextureComp;

	auto l_renderingContext = reinterpret_cast<TAAPassRenderingContext*>(renderingContext);
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();
	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();

	// Use graphics command list to transition resources
	l_graphicsService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Graphics, 0);
	l_graphicsService->TryToTransitState(reinterpret_cast<TextureComponent*>(l_renderingContext->m_input), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	
	// Transition read texture from its current state to ReadOnly
	l_graphicsService->TryToTransitState(l_readTexture, m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	
	l_graphicsService->TryToTransitState(reinterpret_cast<TextureComponent*>(l_renderingContext->m_motionVector), m_CommandListComp_Graphics, Accessibility::WriteOnly, Accessibility::ReadOnly);
	l_graphicsService->TryToTransitState(l_writeTexture, m_CommandListComp_Graphics, Accessibility::ReadOnly, Accessibility::WriteOnly);
	l_graphicsService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);

	l_graphicsService->CommandListBegin(m_RenderPassComp, m_CommandListComp_Compute, 0);
	l_graphicsService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Compute);

	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_renderingContext->m_input, 1);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_readTexture, 2);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_renderingContext->m_motionVector, 3);
	l_graphicsService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Compute, ShaderStage::Compute, l_writeTexture, 4);

	l_graphicsService->Dispatch(m_RenderPassComp, m_CommandListComp_Compute, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	l_graphicsService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Compute);


	return true;
}

RenderPassComponent* TAAPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* TAAPass::GetResult()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_frameCount = l_graphicsService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_OddTextureComp : m_EvenTextureComp;
}

// Must be called in the same frame phase as PrepareCommandList; the returned
// texture is the ping-pong read source for the frame that just ran.
GPUResourceComponent* TAAPass::GetHistory()
{
	auto l_graphicsService = g_Engine->getGraphicsService();
	auto l_frameCount = l_graphicsService->GetFrameCountSinceLaunch();
	auto l_isOddFrame = l_frameCount % 2 == 1;

	return l_isOddFrame ? m_EvenTextureComp : m_OddTextureComp;
}

bool TAAPass::RenderTargetsCreationFunc()
{
	auto l_graphicsService = g_Engine->getGraphicsService();

	if (m_EvenTextureComp)
		l_graphicsService->Delete(m_EvenTextureComp);

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	m_EvenTextureComp = l_graphicsService->AddTextureComponent("TAA Pass Result (Even)/");
	m_EvenTextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_EvenTextureComp->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_graphicsService->Initialize(m_EvenTextureComp);

	if (m_OddTextureComp)
		l_graphicsService->Delete(m_OddTextureComp);

	m_OddTextureComp = l_graphicsService->AddTextureComponent("TAA Pass Result (Odd)/");
	m_OddTextureComp->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_OddTextureComp->m_TextureDesc.Usage = TextureUsage::ColorAttachment;

	l_graphicsService->Initialize(m_OddTextureComp);

	return true;
}