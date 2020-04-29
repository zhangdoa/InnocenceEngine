#include "SunShadowPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace SunShadowPass
{
	bool drawSunShadow();
	bool blur();

	RenderPassDataComponent* m_sunShadowRPDC;
	ShaderProgramComponent* m_sunShadowSPC;
	SamplerDataComponent* m_sunShadowSDC;

	RenderPassDataComponent* m_blurRPDC_Odd;
	RenderPassDataComponent* m_blurRPDC_Even;
	ShaderProgramComponent* m_blurSPC_Odd;
	ShaderProgramComponent* m_blurSPC_Even;
	SamplerDataComponent* m_blurSDC;
}

bool SunShadowPass::Setup()
{
	m_sunShadowSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowPass/");

	m_sunShadowSPC->m_ShaderFilePaths.m_VSPath = "sunShadowPass.vert/";
	m_sunShadowSPC->m_ShaderFilePaths.m_GSPath = "sunShadowPass.geom/";
	m_sunShadowSPC->m_ShaderFilePaths.m_PSPath = "sunShadowPass.frag/";

	m_sunShadowRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("SunShadowPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 2048;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 2048;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 2048;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 2048;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerCullMode = RasterizerCullMode::Front;

	m_sunShadowRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_sunShadowRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 1;

	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 2;

	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 5;

	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceCount = 1;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;
	m_sunShadowRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_sunShadowRPDC->m_ShaderProgram = m_sunShadowSPC;

	m_sunShadowSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("SunShadowPass/");

	m_sunShadowSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_sunShadowSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	//
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_blurSPC_Odd = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowBlurPassOdd/");

	m_blurSPC_Odd->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_blurSPC_Odd->m_ShaderFilePaths.m_GSPath = "sunShadowBlurPass.geom/";
	m_blurSPC_Odd->m_ShaderFilePaths.m_PSPath = "sunShadowBlurPassOdd.frag/";

	m_blurSPC_Even = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowBlurPassEven/");

	m_blurSPC_Even->m_ShaderFilePaths.m_VSPath = "2DImageProcess.vert/";
	m_blurSPC_Even->m_ShaderFilePaths.m_GSPath = "sunShadowBlurPass.geom/";
	m_blurSPC_Even->m_ShaderFilePaths.m_PSPath = "sunShadowBlurPassEven.frag/";

	m_blurRPDC_Odd = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("SunShadowBlurPassOdd/");
	m_blurRPDC_Even = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("SunShadowBlurPassEven/");

	m_blurRPDC_Odd->m_RenderPassDesc = l_RenderPassDesc;
	m_blurRPDC_Even->m_RenderPassDesc = l_RenderPassDesc;

	std::vector<ResourceBinderLayoutDesc> l_ResourceBinderLayoutDescs(3);

	l_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	l_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	l_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	l_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	l_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	l_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;
	l_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	l_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Sampler;
	l_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	l_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	l_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	l_ResourceBinderLayoutDescs.resize(3);

	m_blurRPDC_Odd->m_ResourceBinderLayoutDescs = l_ResourceBinderLayoutDescs;
	m_blurRPDC_Even->m_ResourceBinderLayoutDescs = l_ResourceBinderLayoutDescs;

	m_blurRPDC_Odd->m_ShaderProgram = m_blurSPC_Odd;
	m_blurRPDC_Even->m_ShaderProgram = m_blurSPC_Even;

	m_blurSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("SunShadowBlurPass/");

	m_blurSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Edge;
	m_blurSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Edge;

	return true;
}

bool SunShadowPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_sunShadowSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_sunShadowRPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_sunShadowSDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_blurSPC_Odd);
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_blurSPC_Even);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_blurRPDC_Even);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_blurSDC);

	return true;
}

bool SunShadowPass::drawSunShadow()
{
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_sunShadowRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_sunShadowRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_sunShadowRPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_sunShadowRPDC, ShaderStage::Geometry, l_CSMGBDC->m_ResourceBinder, 2, 5, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_sunShadowRPDC, ShaderStage::Pixel, m_sunShadowSDC->m_ResourceBinder, 4, 0, Accessibility::ReadOnly);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		auto l_visible = static_cast<uint32_t>(l_drawCallData.visibilityMask & VisibilityMask::Sun);
		if (l_visible && l_drawCallData.meshUsage != MeshUsage::Skeletal)
		{
			if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
			{
				if (l_drawCallData.material->m_ShaderModel == ShaderModel::Opaque)
				{
					if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
					{
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_sunShadowRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 0, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_sunShadowRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 1, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_sunShadowRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 3, 0);

						g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_sunShadowRPDC, l_drawCallData.mesh);

						g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_sunShadowRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 3, 0);
					}
				}
			}
		}
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_sunShadowRPDC);

	return true;
}
bool SunShadowPass::blur()
{
	auto l_perFrameGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(ProceduralMeshShape::Square);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_blurRPDC_Odd, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Pixel, l_perFrameGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Pixel, m_sunShadowRPDC->m_RenderTargetsResourceBinders[0], 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Pixel, m_blurSDC->m_ResourceBinder, 2, 0);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_blurRPDC_Odd, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Pixel, m_sunShadowRPDC->m_RenderTargetsResourceBinders[0], 1, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_blurRPDC_Odd);

	//
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_blurRPDC_Even, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_blurRPDC_Even);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_blurRPDC_Even);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Even, ShaderStage::Pixel, l_perFrameGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Even, ShaderStage::Pixel, m_blurRPDC_Odd->m_RenderTargetsResourceBinders[0], 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Even, ShaderStage::Pixel, m_blurSDC->m_ResourceBinder, 2, 0);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_blurRPDC_Even, l_mesh);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blurRPDC_Even, ShaderStage::Pixel, m_blurRPDC_Odd->m_RenderTargetsResourceBinders[0], 1, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_blurRPDC_Even);

	return true;
}

bool SunShadowPass::PrepareCommandList()
{
	drawSunShadow();
	blur();

	return true;
}

bool SunShadowPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_sunShadowRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_sunShadowRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_blurRPDC_Odd);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_blurRPDC_Odd);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_blurRPDC_Even);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_blurRPDC_Even);

	return true;
}

bool SunShadowPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_sunShadowRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_blurRPDC_Even);

	return true;
}

RenderPassDataComponent* SunShadowPass::GetRPDC()
{
	return m_sunShadowRPDC;
}

ShaderProgramComponent* SunShadowPass::GetSPC()
{
	return m_sunShadowSPC;
}

IResourceBinder* SunShadowPass::GetShadowMap()
{
	return m_blurRPDC_Even->m_RenderTargetsResourceBinders[0];
}