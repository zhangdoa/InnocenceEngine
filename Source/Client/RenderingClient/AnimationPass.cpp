#include "AnimationPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

struct AnimationPassConstantBuffer
{
	Mat4 rootOffsetMatrix;
	float duration;
	uint32_t numChannels;
	uint32_t numTicks;
	float currentTime;
	float padding[44];
};

namespace AnimationPass
{
	GPUBufferDataComponent* m_GBDC;
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;
}

bool AnimationPass::Setup()
{
	m_GBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("AnimationInfoCBuffer/");
	m_GBDC->m_ElementCount = 512;
	m_GBDC->m_ElementSize = sizeof(AnimationPassConstantBuffer);
	m_GBDC->m_BindingPoint = 0;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_GBDC);

	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("AnimationPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "animationPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "animationPass.frag/";

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("AnimationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 4;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthClamp = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(12);
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
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorSetIndex = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorIndex = 10;

	m_RPDC->m_ResourceBinderLayoutDescs[10].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorSetIndex = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorIndex = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[10].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBinderLayoutDescs[11].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_DescriptorSetIndex = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_DescriptorIndex = 6;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[11].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("AnimationPass/");

	m_SDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	return true;
}

bool AnimationPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_GBDC);
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	return true;
}

bool AnimationPass::PrepareCommandList()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);
	auto l_AnimationInfo = g_pModuleManager->getRenderingFrontend()->getAnimationInfo("..//Res//ConvertedAssets//Wolf_Wolf_Skeleton-Wolf_Run_Cycle_.InnoAnimation/");

	if (l_AnimationInfo.ADC)
	{
		static float l_currentTime = 0;
		if (l_currentTime < l_AnimationInfo.ADC->m_Duration)
		{
			l_currentTime += 0.5f;
		}
		else
		{
			l_currentTime = 0;
		}

		AnimationPassConstantBuffer cb;
		cb.duration = l_AnimationInfo.ADC->m_Duration;
		cb.numChannels = l_AnimationInfo.ADC->m_NumChannels;
		cb.numTicks = l_AnimationInfo.ADC->m_NumTicks;
		cb.currentTime = l_currentTime / cb.duration;
		cb.rootOffsetMatrix = InnoMath::generateIdentityMatrix<float>();
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_GBDC, &cb, 0, 1);

		g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
		g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
		g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 8, 0);
		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, m_GBDC->m_ResourceBinder, 9, 10, Accessibility::ReadOnly, 0, 1);
		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_AnimationInfo.KeyData->m_ResourceBinder, 10, 5, Accessibility::ReadOnly);

		auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
		auto l_drawCallCount = l_drawCallInfo.size();

		for (uint32_t i = 0; i < l_drawCallCount; i++)
		{
			auto l_drawCallData = l_drawCallInfo[i];
			if (l_drawCallData.visibility == Visibility::Opaque)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated && l_drawCallData.meshUsage == MeshUsage::Skeletal)
				{
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
					{
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 3, 0);
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 4, 1);
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 5, 2);
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 6, 3);
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 7, 4);
					}

					auto l_Skeleton = g_pModuleManager->getRenderingFrontend()->getSkeletonGPUBuffer(l_drawCallData.mesh->m_SDC);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_Skeleton->m_ResourceBinder, 11, 6, Accessibility::ReadOnly);

					g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_drawCallData.mesh);

					if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
					{
						g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 3, 0);
						g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 4, 1);
						g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 5, 2);
						g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 6, 3);
						g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 7, 4);
					}
				}
			}
		}

		g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);
	}

	return true;
}

bool AnimationPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool AnimationPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

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