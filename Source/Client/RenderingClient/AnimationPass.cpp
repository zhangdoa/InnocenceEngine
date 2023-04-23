#include "AnimationPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool AnimationPass::Setup(ISystemConfig *systemConfig)
{
	m_ShaderProgramComp = g_Engine->getRenderingServer()->AddShaderProgramComponent("AnimationPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "animationPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "animationPass.frag/";

	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("AnimationPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 4;
	l_RenderPassDesc.m_RenderTargetsReservationFunc = std::bind(&AnimationPass::RenderTargetsReservationFunc, this);
	l_RenderPassDesc.m_RenderTargetsCreationFunc = std::bind(&AnimationPass::RenderTargetsCreationFunc, this);
	l_RenderPassDesc.m_DepthStencilRenderTargetsCreationFunc = std::bind(&AnimationPass::DepthStencilRenderTargetsCreationFunc, this);

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

	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 10;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 5;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("AnimationPass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool AnimationPass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_ShaderProgramComp);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool AnimationPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus AnimationPass::GetStatus()
{
	return m_ObjectStatus;
}

bool AnimationPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);
	auto l_AnimationGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Animation);

	auto& l_AnimationDrawCallInfo = g_Engine->getRenderingFrontend()->GetAnimationDrawCallInfo();

	if (l_AnimationDrawCallInfo.size())
	{
		g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
		g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
		// Don't clean render targets since they are from previous pass
		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 8);
		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);

		for (auto i : l_AnimationDrawCallInfo)
		{
			g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_AnimationGPUBufferComp, 9, Accessibility::ReadOnly, i.animationConstantBufferIndex, 1);
			g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, i.animationInstance.animationData.keyData, 10, Accessibility::ReadOnly);

			if (i.drawCallInfo.mesh->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, Accessibility::ReadOnly, i.drawCallInfo.meshConstantBufferIndex, 1);
				g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, Accessibility::ReadOnly, i.drawCallInfo.materialConstantBufferIndex, 1);

				if (i.drawCallInfo.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[0].m_Texture, 3);
					g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[1].m_Texture, 4);
					g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[2].m_Texture, 5);
					g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[3].m_Texture, 6);
					g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[4].m_Texture, 7);
				}

				g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp, i.drawCallInfo.mesh);

				if (i.drawCallInfo.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[0].m_Texture, 3);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[1].m_Texture, 4);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[2].m_Texture, 5);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[3].m_Texture, 6);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[4].m_Texture, 7);
				}
			}
		}

		g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);
	}
	else
	{
		g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
		g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
		g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);
	}

	return true;
}

RenderPassComponent* AnimationPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool Inno::AnimationPass::RenderTargetsReservationFunc()
{
	m_RenderPassComp->m_RenderTargets.resize(m_RenderPassComp->m_RenderPassDesc.m_RenderTargetCount);

    return true;
}

bool AnimationPass::RenderTargetsCreationFunc()
{
	for (size_t i = 0; i < m_RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		m_RenderPassComp->m_RenderTargets[i] = OpaquePass::Get().GetRenderPassComp()->m_RenderTargets[i];
	}

	return true;
}

bool AnimationPass::DepthStencilRenderTargetsCreationFunc()
{
	m_RenderPassComp->m_DepthStencilRenderTarget = OpaquePass::Get().GetRenderPassComp()->m_DepthStencilRenderTarget;

	return true;
}