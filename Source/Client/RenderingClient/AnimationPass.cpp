#include "AnimationPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

namespace AnimationPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool AnimationPass::Setup()
{
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("AnimationPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "animationPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "animationPass.frag/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("AnimationPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 4;
	l_RenderPassDesc.m_UseColorBuffer = false;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(11);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_RPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 4;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Sampler;
	m_RPDC->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[8].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 10;

	m_RPDC->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 5;
	m_RPDC->m_ResourceBindingLayoutDescs[10].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBindingLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("AnimationPass/");

	m_SDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	return true;
}

bool AnimationPass::Initialize()
{
	m_RPDC->m_RenderTargets.resize(m_RPDC->m_RenderPassDesc.m_RenderTargetCount);

	for (size_t i = 0; i < m_RPDC->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		m_RPDC->m_RenderTargets[i] = OpaquePass::GetRPDC()->m_RenderTargets[i];
	}

	m_RPDC->m_DepthStencilRenderTarget = OpaquePass::GetRPDC()->m_DepthStencilRenderTarget;

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool AnimationPass::PrepareCommandList()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);
	auto l_AnimationGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Animation);

	auto& l_AnimationDrawCallInfo = g_Engine->getRenderingFrontend()->getAnimationDrawCallInfo();

	if (l_AnimationDrawCallInfo.size())
	{
		g_Engine->getRenderingServer()->BeginCapture();

		g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
		g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
		// Don't clean render targets since they are from previous pass
		g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_SDC, 8);
		g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);

		for (auto i : l_AnimationDrawCallInfo)
		{
			g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_AnimationGBDC, 9, Accessibility::ReadOnly, i.animationConstantBufferIndex, 1);
			g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, i.animationInstance.animationData.keyData, 10, Accessibility::ReadOnly);

			if (i.drawCallInfo.mesh->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_MeshGBDC, 1, Accessibility::ReadOnly, i.drawCallInfo.meshConstantBufferIndex, 1);
				g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC, 2, Accessibility::ReadOnly, i.drawCallInfo.materialConstantBufferIndex, 1);

				if (i.drawCallInfo.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[0].m_Texture, 3);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[1].m_Texture, 4);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[2].m_Texture, 5);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[3].m_Texture, 6);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[4].m_Texture, 7);
				}

				g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RPDC, i.drawCallInfo.mesh);

				if (i.drawCallInfo.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[0].m_Texture, 3);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[1].m_Texture, 4);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[2].m_Texture, 5);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[3].m_Texture, 6);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[4].m_Texture, 7);
				}
			}
		}

		g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

		g_Engine->getRenderingServer()->EndCapture();
	}
	else
	{
		g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
		g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
		g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);
	}

	g_Engine->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_Engine->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool AnimationPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent* AnimationPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent* AnimationPass::GetSPC()
{
	return m_SPC;
}