#include "VXGIGeometryProcessPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "VXGIRenderer.h"
#include "SunShadowBlurEvenPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool VXGIGeometryProcessPass::Setup(ISystemConfig *systemConfig)
{
	auto l_VXGIRenderingConfig = VXGIRenderer::Get().GetVXGIRenderingConfig();
	
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("VXGIGeometryProcessPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "voxelGeometryProcessPass.vert/";
	m_SPC->m_ShaderFilePaths.m_GSPath = "voxelGeometryProcessPass.geom/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "voxelGeometryProcessPass.frag/";

	m_RPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("VXGIGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::Sample;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::R;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UInt32;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_VXGIRenderingConfig.m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_VXGIRenderingConfig.m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = l_VXGIRenderingConfig.m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.UseMipMap = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_VXGIRenderingConfig.m_voxelizationResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_VXGIRenderingConfig.m_voxelizationResolution;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBindingLayoutDescs.resize(13);
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_RPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_RPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 9;

	m_RPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;

	m_RPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 3;
	m_RPDC->m_ResourceBindingLayoutDescs[8].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[8].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 4;
	m_RPDC->m_ResourceBindingLayoutDescs[9].m_SubresourceCount = 1;
	m_RPDC->m_ResourceBindingLayoutDescs[9].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Image;
	m_RPDC->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 2;
	m_RPDC->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 5;
	m_RPDC->m_ResourceBindingLayoutDescs[10].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[11].m_GPUResourceType = GPUResourceType::Sampler;
	m_RPDC->m_ResourceBindingLayoutDescs[11].m_DescriptorSetIndex = 3;
	m_RPDC->m_ResourceBindingLayoutDescs[11].m_DescriptorIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[11].m_IndirectBinding = true;

	m_RPDC->m_ResourceBindingLayoutDescs[12].m_GPUResourceType = GPUResourceType::Buffer;
	m_RPDC->m_ResourceBindingLayoutDescs[12].m_DescriptorSetIndex = 0;
	m_RPDC->m_ResourceBindingLayoutDescs[12].m_DescriptorIndex = 5;

	m_RPDC->m_ShaderProgram = m_SPC;

	m_SDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("VXGIGeometryProcessPass/");

	m_SDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_result = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("VoxelGeometryProcessSBuffer/");
	m_result->m_ElementCount = l_VXGIRenderingConfig.m_voxelizationResolution * l_VXGIRenderingConfig.m_voxelizationResolution * l_VXGIRenderingConfig.m_voxelizationResolution * 2;
	m_result->m_ElementSize = sizeof(uint32_t);
	m_result->m_GPUAccessibility = Accessibility::ReadWrite;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIGeometryProcessPass::Initialize()
{	
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_result);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIGeometryProcessPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);

	g_Engine->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_RPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_SDC, 11);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Geometry, VXGIRenderer::Get().GetVoxelizationCBuffer(), 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, VXGIRenderer::Get().GetVoxelizationCBuffer(), 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, m_result, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, SunShadowBlurEvenPass::Get().GetResult(), 10, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_CSMGBDC, 12, Accessibility::ReadOnly);

	auto &l_drawCallInfo = g_Engine->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Opaque)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Vertex, l_MeshGBDC, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture, 5);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 6);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture, 7);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture, 8);
					g_Engine->getRenderingServer()->BindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture, 9);

					g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RPDC, l_drawCallData.mesh);

					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture, 5);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 6);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture, 7);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture, 8);
					g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture, 9);
				}
			}
		}
	}

	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, m_result, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_RPDC, ShaderStage::Pixel, SunShadowBlurEvenPass::Get().GetResult(), 10, Accessibility::ReadOnly);

	g_Engine->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

RenderPassDataComponent* VXGIGeometryProcessPass::GetRPDC()
{
	return m_RPDC;
}

GPUResourceComponent* VXGIGeometryProcessPass::GetResult()
{
	return m_result;
}