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

	m_RPDC->m_ResourceBinderLayoutDescs.resize(11);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorIndex = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_ResourceCount = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[7].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[8].m_IndirectBinding = true;

	m_RPDC->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorIndex = 10;

	m_RPDC->m_ResourceBinderLayoutDescs[10].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorIndex = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;

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
		g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 8, 0);
		g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);

		for (auto i : l_AnimationDrawCallInfo)
		{
			g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_AnimationGBDC->m_ResourceBinder, 9, 10, Accessibility::ReadOnly, i.animationConstantBufferIndex, 1);
			g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, i.animationInstance.animationData.keyData->m_ResourceBinder, 10, 5, Accessibility::ReadOnly);

			if (i.drawCallInfo.mesh->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, i.drawCallInfo.meshConstantBufferIndex, 1);
				g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, i.drawCallInfo.materialConstantBufferIndex, 1);

				if (i.drawCallInfo.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 3, 0);
					g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 4, 1);
					g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 5, 2);
					g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 6, 3);
					g_Engine->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 7, 4);
				}

				g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RPDC, i.drawCallInfo.mesh);

				if (i.drawCallInfo.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 3, 0);
					g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 4, 1);
					g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 5, 2);
					g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 6, 3);
					g_Engine->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, i.drawCallInfo.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 7, 4);
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