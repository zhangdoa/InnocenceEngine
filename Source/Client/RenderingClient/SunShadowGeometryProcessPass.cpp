#include "SunShadowGeometryProcessPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "../../Engine/Engine.h"

using namespace Inno;


using namespace DefaultGPUBuffers;

bool SunShadowGeometryProcessPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_shadowMapResolution = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().shadowMapResolution;

	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("SunShadowGeometryProcessPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "sunShadowGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_GSPath = "sunShadowGeometryProcessPass.geom/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "sunShadowGeometryProcessPass.frag/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("SunShadowGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_Resizable = false;
	
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

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(5);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 1;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 2;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 5;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("SunShadowGeometryProcessPass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool SunShadowGeometryProcessPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeSamplerComponent(m_SamplerComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool SunShadowGeometryProcessPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus SunShadowGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool SunShadowGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);
	auto l_CSMGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::CSM);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);

	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Geometry, l_CSMGPUBufferComp, 2);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 4);

	auto& l_drawCallInfo = g_Engine->Get<RenderingContextService>()->GetDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::Sun);
		if (l_visible && l_drawCallData.meshUsage != MeshUsage::Skeletal)
		{
			if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
			{
				if (l_drawCallData.material->m_ShaderModel == ShaderModel::Opaque)
				{
					if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
					{
						l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 0, l_drawCallData.meshConstantBufferIndex, 1);
						l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 1, l_drawCallData.materialConstantBufferIndex, 1);

						l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 3);

						l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, l_drawCallData.mesh);

						l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 3);
					}
				}
			}
		}
	}
	
	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* SunShadowGeometryProcessPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* SunShadowGeometryProcessPass::GetResult()
{
	return m_RenderPassComp->m_RenderTargets[0].m_Texture;
}

uint32_t SunShadowGeometryProcessPass::GetShadowMapResolution()
{
	return m_shadowMapResolution;
}