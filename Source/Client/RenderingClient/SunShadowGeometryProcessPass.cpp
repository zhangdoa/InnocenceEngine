#include "SunShadowGeometryProcessPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool SunShadowGeometryProcessPass::Setup(ISystemConfig *systemConfig)
{	
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("SunShadowGeometryProcessPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "sunShadowGeometryProcessPass.vert/";
	m_SPC->m_ShaderFilePaths.m_GSPath = "sunShadowGeometryProcessPass.geom/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "sunShadowGeometryProcessPass.frag/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("SunShadowGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

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

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(5);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 1;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 2;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 5;

	m_RPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Sampler;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("SunShadowGeometryProcessPass/");

	m_SDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowGeometryProcessPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool SunShadowGeometryProcessPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);

	// Only clear the RT when a round finished
	if(g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer().activeCascade == 0)
	{
		g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);
	}

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Geometry, l_CSMGBDC, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_SDC, 4, Accessibility::ReadOnly);

	auto& l_drawCallInfo = g_Engine->getRenderingFrontend()->getDrawCallInfo();
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
						g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_MeshGBDC, 0, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
						g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC, 1, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

						g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 3);

						g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RPDC, l_drawCallData.mesh);

						g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 3);
					}
				}
			}
		}
	}

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* SunShadowGeometryProcessPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* SunShadowGeometryProcessPass::GetResult()
{
	return m_RPDC->m_RenderTargets[0];
}

uint32_t SunShadowGeometryProcessPass::GetShadowMapResolution()
{
	return m_shadowMapResolution;
}