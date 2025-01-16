#include "VXGIGeometryProcessPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingFrontend.h"

#include "VXGIRenderer.h"

#include "../../Engine/Engine.h"

using namespace Inno;


using namespace DefaultGPUBuffers;

bool VXGIGeometryProcessPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_VXGIRenderingConfig = &reinterpret_cast<VXGIRendererSystemConfig*>(systemConfig)->m_VXGIRenderingConfig;
	
	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("VXGIGeometryProcessPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "voxelGeometryProcessPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_GSPath = "voxelGeometryProcessPass.geom/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "voxelGeometryProcessPass.frag/";

	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("VXGIGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingFrontend>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_Resizable = false;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::Sample;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::R;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UInt32;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_VXGIRenderingConfig->m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_VXGIRenderingConfig->m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = l_VXGIRenderingConfig->m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.UseMipMap = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_VXGIRenderingConfig->m_voxelizationResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_VXGIRenderingConfig->m_voxelizationResolution;

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

	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 9;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[8].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Image;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 2;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 4;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_SubresourceCount = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[9].m_IndirectBinding = true;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_GPUResourceType = GPUResourceType::Sampler;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorSetIndex = 3;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[10].m_IndirectBinding = true;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_SamplerComp = l_renderingServer->AddSamplerComponent("VXGIGeometryProcessPass/");

	m_SamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_SamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	m_result = l_renderingServer->AddGPUBufferComponent("VoxelGeometryProcessSBuffer/");
	m_result->m_ElementCount = l_VXGIRenderingConfig->m_voxelizationResolution * l_VXGIRenderingConfig->m_voxelizationResolution * l_VXGIRenderingConfig->m_voxelizationResolution * 2;
	m_result->m_ElementSize = sizeof(uint32_t);
	m_result->m_GPUAccessibility = Accessibility::ReadWrite;

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool VXGIGeometryProcessPass::Initialize()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);
	l_renderingServer->InitializeSamplerComponent(m_SamplerComp);

	l_renderingServer->InitializeGPUBufferComponent(m_result);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool VXGIGeometryProcessPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus VXGIGeometryProcessPass::GetStatus()
{
	return m_ObjectStatus;
}

bool VXGIGeometryProcessPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);

	l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
	l_renderingServer->ClearRenderTargets(m_RenderPassComp);

	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Geometry, VXGIRenderer::Get().GetVoxelizationCBuffer(), 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, VXGIRenderer::Get().GetVoxelizationCBuffer(), 3);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_result, 4);
	l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_SamplerComp, 10);

	auto &l_drawCallInfo = g_Engine->Get<RenderingFrontend>()->GetDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		auto l_visible = static_cast<uint32_t>(l_drawCallData.visibilityMask & VisibilityMask::MainCamera);
		if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Opaque)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.meshConstantBufferIndex, 1);
					l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.materialConstantBufferIndex, 1);

					l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture, 5);
					l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 6);
					l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture, 7);
					l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture, 8);
					l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture, 9);

					l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, l_drawCallData.mesh);

					l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture, 5);
					l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture, 6);
					l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture, 7);
					l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture, 8);
					l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture, 9);
				}
			}
		}
	}

	l_renderingServer->UnbindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_result, 4);

	l_renderingServer->CommandListEnd(m_RenderPassComp);

	return true;
}

RenderPassComponent* VXGIGeometryProcessPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

GPUResourceComponent* VXGIGeometryProcessPass::GetResult()
{
	return m_result;
}