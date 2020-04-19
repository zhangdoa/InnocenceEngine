#include "VoxelizationPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace VoxelizationPass
{
	bool setupVoxelizationPass();
	bool setupVisualizationPass();

	bool voxelization();
	bool visualization();

	GPUBufferDataComponent* m_voxelizationCBufferGBDC;

	RenderPassDataComponent* m_voxelizationRPDC;
	ShaderProgramComponent* m_voxelizationSPC;
	SamplerDataComponent* m_voxelizationSDC;

	RenderPassDataComponent* m_visualizationRPDC;
	ShaderProgramComponent* m_visualizationSPC;

	uint32_t voxelizationResolution = 128;
}

bool VoxelizationPass::setupVoxelizationPass()
{
	m_voxelizationCBufferGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VoxelizationPassCBuffer/");
	m_voxelizationCBufferGBDC->m_ElementCount = 1;
	m_voxelizationCBufferGBDC->m_ElementSize = sizeof(VoxelizationConstantBuffer);
	m_voxelizationCBufferGBDC->m_BindingPoint = 11;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_voxelizationCBufferGBDC);

	////
	m_voxelizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelizationPass/");

	m_voxelizationSPC->m_ShaderFilePaths.m_VSPath = "voxelizationPass.vert/";
	m_voxelizationSPC->m_ShaderFilePaths.m_GSPath = "voxelizationPass.geom/";
	m_voxelizationSPC->m_ShaderFilePaths.m_PSPath = "voxelizationPass.frag/";

	m_voxelizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelizationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::Sample;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::R;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UInt32;
	l_RenderPassDesc.m_RenderTargetDesc.Width = voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.UseMipMap = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)voxelizationResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)voxelizationResolution;

	m_voxelizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs.resize(11);
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 9;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 4;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 5;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 1;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceCount = 1;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[5].m_IndirectBinding = true;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 6;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 2;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceCount = 1;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorSetIndex = 7;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorIndex = 3;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[7].m_ResourceCount = 1;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[7].m_IndirectBinding = true;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Image;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorSetIndex = 8;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorIndex = 4;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[8].m_ResourceCount = 1;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[8].m_IndirectBinding = true;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Image;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorSetIndex = 9;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorIndex = 5;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[9].m_ResourceCount = 1;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[9].m_IndirectBinding = true;

	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[10].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorSetIndex = 10;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorIndex = 0;
	m_voxelizationRPDC->m_ResourceBinderLayoutDescs[10].m_IndirectBinding = true;

	m_voxelizationRPDC->m_ShaderProgram = m_voxelizationSPC;

	m_voxelizationSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VoxelizationPass/");

	m_voxelizationSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_voxelizationSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	return true;
}

bool VoxelizationPass::setupVisualizationPass()
{
	m_visualizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelVisualizationPass/");

	m_visualizationSPC->m_ShaderFilePaths.m_VSPath = "voxelVisualizationPass.vert/";
	m_visualizationSPC->m_ShaderFilePaths.m_GSPath = "voxelVisualizationPass.geom/";
	m_visualizationSPC->m_ShaderFilePaths.m_PSPath = "voxelVisualizationPass.frag/";

	m_visualizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelVisualizationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::ColorAttachment;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_viewportSize.x;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_viewportSize.y;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_PrimitiveTopology = PrimitiveTopology::Point;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_viewportSize.x;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_viewportSize.y;

	m_visualizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs.resize(3);

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 9;

	m_visualizationRPDC->m_ShaderProgram = m_visualizationSPC;

	return true;
}

bool VoxelizationPass::Setup()
{
	setupVoxelizationPass();
	setupVisualizationPass();

	return true;
}

bool VoxelizationPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_voxelizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_voxelizationRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_visualizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_visualizationRPDC);

	return true;
}

bool VoxelizationPass::voxelization()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_voxelizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_voxelizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_voxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, m_voxelizationSDC->m_ResourceBinder, 10, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Geometry, m_voxelizationCBufferGBDC->m_ResourceBinder, 3, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, m_voxelizationCBufferGBDC->m_ResourceBinder, 3, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, m_voxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadWrite);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.visibility == Visibility::Opaque)
		{
			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

				if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 5, 0);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 6, 1);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 7, 2);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 8, 3);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 9, 4);
				}

				g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_voxelizationRPDC, l_drawCallData.mesh);

				if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 5, 0);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 6, 1);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 7, 2);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 8, 3);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 9, 4);
				}
			}
		}
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_voxelizationRPDC, ShaderStage::Pixel, m_voxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_voxelizationRPDC);

	return true;
}

bool VoxelizationPass::visualization()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_visualizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_visualizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_visualizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, m_voxelizationRPDC->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Geometry, l_PerFrameCBufferGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, m_voxelizationCBufferGBDC->m_ResourceBinder, 2, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Geometry, m_voxelizationCBufferGBDC->m_ResourceBinder, 2, 9, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_visualizationRPDC, voxelizationResolution * voxelizationResolution * voxelizationResolution);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, m_voxelizationRPDC->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_visualizationRPDC);

	return true;
}

bool VoxelizationPass::PrepareCommandList()
{
	auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getTotalSceneAABB();

	VoxelizationConstantBuffer l_voxelPassCB;
	l_voxelPassCB.volumeCenter = l_sceneAABB.m_center;
	l_voxelPassCB.volumeExtend = l_sceneAABB.m_extend;
	l_voxelPassCB.voxelResolution = Vec4((float)voxelizationResolution, (float)voxelizationResolution, (float)voxelizationResolution, 1.0f);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_voxelizationCBufferGBDC, &l_voxelPassCB);

	voxelization();
	visualization();

	return true;
}

bool VoxelizationPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_voxelizationRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_voxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_visualizationRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_visualizationRPDC);

	return true;
}

bool VoxelizationPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_voxelizationRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_visualizationRPDC);

	return true;
}

IResourceBinder* VoxelizationPass::GetVisualizationResult()
{
	return m_visualizationRPDC->m_RenderTargets[0]->m_ResourceBinder;
}