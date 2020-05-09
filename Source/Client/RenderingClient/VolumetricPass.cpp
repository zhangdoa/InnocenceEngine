#include "VolumetricPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"
#include "SunShadowPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace VolumetricPass
{
	bool setupGeometryProcessPass();
	bool setupIrradianceInjectionPass();
	bool setupRayMarchingPass();
	bool setupVisualizationPass();

	bool froxelization();
	bool irraidanceInjection();
	bool rayMarching();
	bool visualization(IResourceBinder* input);

	SamplerDataComponent* m_SDC;

	RenderPassDataComponent* m_froxelizationRPDC;
	ShaderProgramComponent* m_froxelizationSPC;

	RenderPassDataComponent* m_visualizationRPDC;
	ShaderProgramComponent* m_visualizationSPC;

	RenderPassDataComponent* m_irraidanceInjectionRPDC;
	ShaderProgramComponent* m_irraidanceInjectionSPC;

	RenderPassDataComponent* m_rayMarchingRPDC;
	ShaderProgramComponent* m_rayMarchingSPC;

	TextureDataComponent* m_irraidanceInjectionResult;
	TextureDataComponent* m_rayMarchingResult;

	uint32_t m_voxelizationResolution = 64;
}

bool VolumetricPass::setupGeometryProcessPass()
{
	m_froxelizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricGeometryProcessPass/");

	m_froxelizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricGeometryProcessPass.vert/";
	m_froxelizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricGeometryProcessPass.geom/";
	m_froxelizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricGeometryProcessPass.frag/";

	m_froxelizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricGeometryProcessPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::Sample;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = m_voxelizationResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_voxelizationResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_voxelizationResolution;

	m_froxelizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_froxelizationRPDC->m_ShaderProgram = m_froxelizationSPC;

	return true;
}

bool VolumetricPass::setupIrradianceInjectionPass()
{
	m_irraidanceInjectionSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricIrraidanceInjectionPass/");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricIrraidanceInjectionPass.comp/";

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_irraidanceInjectionRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricIrraidanceInjectionPass/");

	m_irraidanceInjectionRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs.resize(7);
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 5;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 6;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 3;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ShaderProgram = m_irraidanceInjectionSPC;

	return true;
}

bool VolumetricPass::setupRayMarchingPass()
{
	m_rayMarchingSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricRayMarchingPass.comp/";

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_rayMarchingRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricRayMarchingPass/");

	m_rayMarchingRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 6;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;

	m_rayMarchingRPDC->m_ShaderProgram = m_rayMarchingSPC;

	return true;
}

bool VolumetricPass::setupVisualizationPass()
{
	m_visualizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricVisualizationPass/");

	m_visualizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricVisualizationPass.vert/";
	m_visualizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricVisualizationPass.frag/";

	m_visualizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricVisualizationPass/");

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
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_viewportSize.x;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_viewportSize.y;

	m_visualizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadOnly;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 4;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[4].m_IndirectBinding = true;

	m_visualizationRPDC->m_ShaderProgram = m_visualizationSPC;

	return true;
}

bool VolumetricPass::Setup()
{
	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VolumetricPass/");

	setupGeometryProcessPass();
	setupIrradianceInjectionPass();
	setupRayMarchingPass();
	setupVisualizationPass();

	////
	auto l_textureDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc().m_RenderTargetDesc;
	l_textureDesc.Sampler = TextureSampler::Sampler3D;
	l_textureDesc.Usage = TextureUsage::Sample;
	l_textureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_textureDesc.Width = m_voxelizationResolution;
	l_textureDesc.Height = m_voxelizationResolution;
	l_textureDesc.DepthOrArraySize = m_voxelizationResolution;

	m_irraidanceInjectionResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricIrraidanceInjectionResult/");
	m_irraidanceInjectionResult->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricRayMarchingResult/");
	m_rayMarchingResult->m_TextureDesc = l_textureDesc;

	return true;
}

bool VolumetricPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_froxelizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_irraidanceInjectionSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_rayMarchingSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_visualizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_visualizationRPDC);

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_irraidanceInjectionResult);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_rayMarchingResult);

	return true;
}

bool VolumetricPass::froxelization()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_froxelizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 3, 0, Accessibility::ReadWrite);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelizationRPDC, l_drawCallData.mesh);
				}
			}
		}
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 3, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_froxelizationRPDC);

	return true;
}

bool VolumetricPass::irraidanceInjection()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = m_voxelizationResolution;
	auto l_numThreadsY = m_voxelizationResolution;
	auto l_numThreadsZ = m_voxelizationResolution;

	DispatchParamsConstantBuffer l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadsX / 8, l_numThreadsY / 8, l_numThreadsZ / 8, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_irraidanceInjectionWorkload, 6, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_irraidanceInjectionRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_SDC->m_ResourceBinder, 6, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_CSMGBDC->m_ResourceBinder, 1, 5, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 2, 6, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, SunShadowPass::GetShadowMap(), 5, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_irraidanceInjectionRPDC, 8, 8, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 3, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, SunShadowPass::GetShadowMap(), 5, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_irraidanceInjectionRPDC);

	return true;
}

bool VolumetricPass::rayMarching()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = m_voxelizationResolution;
	auto l_numThreadsY = m_voxelizationResolution;
	auto l_numThreadsZ = m_voxelizationResolution;

	DispatchParamsConstantBuffer l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadsX / 8, l_numThreadsY / 8, l_numThreadsZ / 8, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_rayMarchingWorkload, 7, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_rayMarchingRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_SDC->m_ResourceBinder, 3, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 6, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 4, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_rayMarchingRPDC, 8, 8, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_rayMarchingRPDC);

	return true;
}

bool VolumetricPass::visualization(IResourceBinder* input)
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_visualizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_visualizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_visualizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 4, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Pixel, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Pixel, input, 3, 0, Accessibility::ReadOnly);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_visualizationRPDC, l_drawCallData.mesh);
				}
			}
		}
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_visualizationRPDC, ShaderStage::Pixel, input, 3, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_visualizationRPDC);

	return true;
}

bool VolumetricPass::Render(bool visualize)
{
	froxelization();
	irraidanceInjection();
	rayMarching();

	if (visualize)
	{
		visualization(m_rayMarchingResult->m_ResourceBinder);
	}

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_rayMarchingRPDC);

	if (visualize)
	{
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_visualizationRPDC);
		g_pModuleManager->getRenderingServer()->WaitForFrame(m_visualizationRPDC);
	}

	return true;
}

bool VolumetricPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_visualizationRPDC);

	return true;
}

IResourceBinder* VolumetricPass::GetRayMarchingResult()
{
	return m_rayMarchingResult->m_ResourceBinder;
}

IResourceBinder* VolumetricPass::GetVisualizationResult()
{
	return m_visualizationRPDC->m_RenderTargets[0]->m_ResourceBinder;
}