#include "SunShadowPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace SunShadowPass
{
	void setupGeometryProcessPass();
	void setupBlurPass();

	bool geometryProcess();
	bool blur();

	RenderPassDataComponent* m_geometryProcessRPDC;
	ShaderProgramComponent* m_geometryProcessSPC;
	SamplerDataComponent* m_geometryProcessSDC;

	RenderPassDataComponent* m_blurRPDC_Odd;
	RenderPassDataComponent* m_blurRPDC_Even;
	ShaderProgramComponent* m_blurSPC_Odd;
	ShaderProgramComponent* m_blurSPC_Even;
	TextureDataComponent* m_oddTDC;
	TextureDataComponent* m_evenTDC;

	uint32_t m_shadowMapResolution = 1024;
}

void SunShadowPass::setupGeometryProcessPass()
{
	m_geometryProcessSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowGeometryProcessPass/");

	m_geometryProcessSPC->m_ShaderFilePaths.m_VSPath = "sunShadowGeometryProcessPass.vert/";
	m_geometryProcessSPC->m_ShaderFilePaths.m_GSPath = "sunShadowGeometryProcessPass.geom/";
	m_geometryProcessSPC->m_ShaderFilePaths.m_PSPath = "sunShadowGeometryProcessPass.frag/";

	m_geometryProcessRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("SunShadowGeometryProcessPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = m_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = m_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_shadowMapResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_shadowMapResolution;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerCullMode = RasterizerCullMode::Front;

	m_geometryProcessRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 1;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 2;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 5;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceCount = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ShaderProgram = m_geometryProcessSPC;
}

void SunShadowPass::setupBlurPass()
{
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = m_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = m_shadowMapResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::Float32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;

	m_blurSPC_Odd = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowBlurPassOdd/");
	m_blurSPC_Odd->m_ShaderFilePaths.m_CSPath = "sunShadowBlurPassOdd.comp/";

	m_blurSPC_Even = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowBlurPassEven/");
	m_blurSPC_Even->m_ShaderFilePaths.m_CSPath = "sunShadowBlurPassEven.comp/";

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
	l_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadOnly;
	l_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	l_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	l_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	l_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	l_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	l_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	l_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	l_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_blurRPDC_Odd->m_ResourceBinderLayoutDescs = l_ResourceBinderLayoutDescs;
	m_blurRPDC_Even->m_ResourceBinderLayoutDescs = l_ResourceBinderLayoutDescs;

	m_blurRPDC_Odd->m_ShaderProgram = m_blurSPC_Odd;
	m_blurRPDC_Even->m_ShaderProgram = m_blurSPC_Even;

	m_oddTDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("SunShadowBlurPassOdd/");
	m_evenTDC = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("SunShadowBlurPassEven/");

	m_oddTDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
	m_evenTDC->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;
}

bool SunShadowPass::Setup()
{
	setupGeometryProcessPass();

	setupBlurPass();

	m_geometryProcessSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("SunShadowGeometryProcessPass/");

	m_geometryProcessSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_geometryProcessSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	return true;
}

bool SunShadowPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_geometryProcessSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_geometryProcessSDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_blurSPC_Odd);
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_blurSPC_Even);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_blurRPDC_Even);

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_oddTDC);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_evenTDC);

	return true;
}

bool SunShadowPass::geometryProcess()
{
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_geometryProcessRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Geometry, l_CSMGBDC->m_ResourceBinder, 2, 5, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessSDC->m_ResourceBinder, 4, 0, Accessibility::ReadOnly);

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
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 0, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 1, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

						g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 3, 0);

						g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_geometryProcessRPDC, l_drawCallData.mesh);

						g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 3, 0);
					}
				}
			}
		}
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_geometryProcessRPDC);

	return true;
}
bool SunShadowPass::blur()
{
	auto l_perFrameGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_blurRPDC_Odd, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Compute, l_perFrameGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Compute, m_geometryProcessRPDC->m_RenderTargetsResourceBinders[0], 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Compute, m_oddTDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_blurRPDC_Odd, m_shadowMapResolution / 8, m_shadowMapResolution / 8, 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Compute, m_geometryProcessRPDC->m_RenderTargetsResourceBinders[0], 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blurRPDC_Odd, ShaderStage::Compute, m_oddTDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_blurRPDC_Odd);

	//
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_blurRPDC_Even, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_blurRPDC_Even);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_blurRPDC_Even);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Even, ShaderStage::Compute, l_perFrameGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Even, ShaderStage::Compute, m_oddTDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Even, ShaderStage::Compute, m_evenTDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_blurRPDC_Even, m_shadowMapResolution / 8, m_shadowMapResolution / 8, 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_blurRPDC_Even, ShaderStage::Compute, m_oddTDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_blurRPDC_Even, ShaderStage::Compute, m_evenTDC->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_blurRPDC_Even);

	return true;
}

bool SunShadowPass::PrepareCommandList()
{
	geometryProcess();
	blur();

	return true;
}

bool SunShadowPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_blurRPDC_Odd);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_blurRPDC_Even);

	return true;
}

RenderPassDataComponent* SunShadowPass::GetGeometryProcessRPDC()
{
	return m_geometryProcessRPDC;
}

RenderPassDataComponent* SunShadowPass::GetBlurRPDCOdd()
{
	return m_blurRPDC_Odd;
}

RenderPassDataComponent* SunShadowPass::GetBlurRPDCEven()
{
	return m_blurRPDC_Even;
}

ShaderProgramComponent* SunShadowPass::GetSPC()
{
	return m_geometryProcessSPC;
}

IResourceBinder* SunShadowPass::GetShadowMap()
{
	return m_evenTDC->m_ResourceBinder;
}