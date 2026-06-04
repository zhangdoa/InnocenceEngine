#include "AnimationPass.h"
#include "OpaquePass.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/DrawCallService.h"
#include "../../Engine/Services/AnimationDrawCallService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
#include "../../Engine/Services/FrameManagementService.h"

using namespace Inno;

bool AnimationPass::Setup(IServiceConfig* systemConfig)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("AnimationPass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "animationPass.vert";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "animationPass.frag";

	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("AnimationPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 4;
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&AnimationPass::RenderTargetsReservationFunc, this);
	l_RenderPassDesc.m_RenderTargetsInitializationFunc = std::bind(&AnimationPass::RenderTargetsCreationFunc, this);
	l_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc = std::bind(&AnimationPass::DepthStencilRenderTargetsReservationFunc, this);
	l_RenderPassDesc.m_DepthStencilRenderTargetsInitializationFunc = std::bind(&AnimationPass::DepthStencilRenderTargetsCreationFunc, this);

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(11);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Vertex;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_IndirectBinding = true;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_ShaderStage = ShaderStage::Pixel;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 10;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = g_Engine->Get<SamplerResourceService>()->Add("AnimationPass");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_CommandListComp_Graphics = g_Engine->Get<CommandListResourceService>()->Add("AnimationPass/Graphics");
	m_CommandListComp_Graphics->m_Type = GPUEngineType::Graphics;

	m_ObjectStatus = ObjectStatus::Created;

	return true;
}

bool AnimationPass::Initialize()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);
	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_CommandListComp_Graphics);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool AnimationPass::Terminate()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	g_Engine->Get<SamplerResourceService>()->Delete(m_SamplerComp);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);
	g_Engine->Get<ShaderProgramResourceService>()->Delete(m_ShaderProgramComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus AnimationPass::GetStatus()
{
	return m_ObjectStatus;
}

bool AnimationPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_MeshGPUBufferComp = g_Engine->Get<DrawCallService>()->GetGPUModelDataBuffer();
	auto l_MaterialGPUBufferComp = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();
	auto l_AnimationGPUBufferComp = g_Engine->Get<AnimationDrawCallService>()->GetAnimationBuffer();

	auto& l_AnimationDrawCallInfo = g_Engine->Get<AnimationDrawCallService>()->GetAnimationDrawCallInfo();
	// m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResource = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	// m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResource = m_SamplerComp;
	// if (l_AnimationDrawCallInfo.size())
	// {
	// 	l_fmService->CommandListBegin(m_CommandListComp_Graphics, m_RenderPassComp, 0);
	// 	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Graphics);
	// 	// Don't clean render targets since they are from previous pass

	// 	for (auto i : l_AnimationDrawCallInfo)
	// 	{
	// 		l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex, l_AnimationGPUBufferComp, 9, i.animationConstantBufferIndex, 1);
	// 		l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex, i.animationInstance.animationData.keyData, 10);

	// 		if (i.drawCallInfo.mesh->m_ObjectStatus == ObjectStatus::Activated)
	// 		{
	// 			l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, i.drawCallInfo.m_PerObjectConstantBufferIndex, 1);
	// 			l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, i.drawCallInfo.m_PerObjectConstantBufferIndex, 1);

	// 			if (i.drawCallInfo.material->m_ObjectStatus == ObjectStatus::Activated)
	// 			{
	// 				l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[0], 3);
	// 				l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[1], 4);
	// 				l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[2], 5);
	// 				l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[3], 6);
	// 				l_fmService->BindGPUResource(m_RenderPassComp, m_CommandListComp_Graphics, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[4], 7);
	// 			}

	// 			l_fmService->DrawIndexedInstanced(m_RenderPassComp, m_CommandListComp_Graphics, i.drawCallInfo.mesh);
	// 		}
	// 	}

	// 	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);
	// }
	// else
	// {
	// 	l_fmService->CommandListBegin(m_CommandListComp_Graphics, m_RenderPassComp, 0);
	// 	l_fmService->BindRenderPassComponent(m_RenderPassComp, m_CommandListComp_Graphics);
	// 	l_fmService->CommandListEnd(m_RenderPassComp, m_CommandListComp_Graphics);
	// }

	return false;
}

RenderPassComponent* AnimationPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool AnimationPass::RenderTargetsReservationFunc()
{
	auto l_fmService = g_Engine->Get<FrameManagementService>();
	if (m_RenderPassComp->m_OutputMergerTarget == nullptr)
		g_Engine->Get<RenderPassResourceService>()->Add(m_RenderPassComp->m_OutputMergerTarget);

	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	l_outputMergerTarget->m_ColorOutputs.resize(m_RenderPassComp->m_RenderPassDesc.m_RenderTargetCount);

	return true;
}

bool AnimationPass::RenderTargetsCreationFunc()
{
	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	for (size_t i = 0; i < l_outputMergerTarget->m_ColorOutputs.size(); i++)
	{
		l_outputMergerTarget->m_ColorOutputs[i] = OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_ColorOutputs[i];
	}

	return true;
}

bool AnimationPass::DepthStencilRenderTargetsReservationFunc()
{
	return true;
}

bool AnimationPass::DepthStencilRenderTargetsCreationFunc()
{
	auto l_outputMergerTarget = m_RenderPassComp->m_OutputMergerTarget;
	l_outputMergerTarget->m_DepthStencilOutput = OpaquePass::Get().GetRenderPassComp()->m_OutputMergerTarget->m_DepthStencilOutput;
	return true;
}