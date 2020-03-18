#include "VolumetricFogPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace VolumetricFogPass
{
	bool setupFroxelizationPass();
	bool setupFroxelVisualizationPass();
	bool setupIrradianceInjectionPass();
	bool setupRayMarchingPass();

	bool froxelization();
	bool froxelVisualization();
	bool irraidanceInjection();
	bool rayMarching();

	RenderPassDataComponent* m_froxelizationRPDC;
	ShaderProgramComponent* m_froxelizationSPC;

	RenderPassDataComponent* m_froxelVisualizationRPDC;
	ShaderProgramComponent* m_froxelVisualizationSPC;

	RenderPassDataComponent* m_irraidanceInjectionRPDC;
	ShaderProgramComponent* m_irraidanceInjectionSPC;

	RenderPassDataComponent* m_rayMarchingRPDC;
	ShaderProgramComponent* m_rayMarchingSPC;
	SamplerDataComponent* m_rayMarchingSDC;

	TextureDataComponent* m_irraidanceInjectionResult;
	TextureDataComponent* m_rayMarchingResult;
}

bool VolumetricFogPass::setupFroxelizationPass()
{
	m_froxelizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogFroxelizationPass/");

	m_froxelizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricFogFroxelizationPass.vert/";
	m_froxelizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricFogFroxelizationPass.geom/";
	m_froxelizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricFogFroxelizationPass.frag/";

	m_froxelizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogFroxelizationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::RawImage;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 64;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 64;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 64;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 64;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 64;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_MaxDepth = 64;

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

bool VolumetricFogPass::setupFroxelVisualizationPass()
{
	m_froxelVisualizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFroxelVisualizationPass/");

	m_froxelVisualizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricFogFroxelVisualizationPass.vert/";
	m_froxelVisualizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricFogFroxelVisualizationPass.geom/";
	m_froxelVisualizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricFogFroxelVisualizationPass.frag/";

	m_froxelVisualizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogFroxelVisualizationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler2D;
	l_RenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::ColorAttachment;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_viewportSize.x;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_viewportSize.y;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = l_viewportSize.x;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = l_viewportSize.y;

	m_froxelVisualizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_froxelVisualizationRPDC->m_ResourceBinderLayoutDescs.resize(1);

	m_froxelVisualizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_froxelVisualizationRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_froxelVisualizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelVisualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_froxelVisualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_froxelVisualizationRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_froxelVisualizationRPDC->m_ShaderProgram = m_froxelVisualizationSPC;

	return true;
}

bool VolumetricFogPass::setupIrradianceInjectionPass()
{
	m_irraidanceInjectionSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogIrraidanceInjectionPass/");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricFogIrraidanceInjectionPass.comp/";

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_irraidanceInjectionRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogIrraidanceInjectionPass/");

	m_irraidanceInjectionRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs.resize(3);
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 6;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ShaderProgram = m_irraidanceInjectionSPC;

	return true;
}

bool VolumetricFogPass::setupRayMarchingPass()
{
	m_rayMarchingSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricFogRayMarchingPass.comp/";

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_rayMarchingRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogRayMarchingPass/");

	m_rayMarchingRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs.resize(4);
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

	m_rayMarchingRPDC->m_ShaderProgram = m_rayMarchingSPC;

	m_rayMarchingSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VolumetricFogRayMarchingPass/");

	return true;
}

bool VolumetricFogPass::Setup()
{
	setupFroxelizationPass();
	setupFroxelVisualizationPass();
	setupIrradianceInjectionPass();
	setupRayMarchingPass();

	////
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::RawImage;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 64;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 64;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 64;

	m_irraidanceInjectionResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricFogIrraidanceInjectionResult/");
	m_irraidanceInjectionResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_rayMarchingResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricFogRayMarchingResult/");
	m_rayMarchingResult->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	return true;
}

bool VolumetricFogPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_froxelizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_froxelVisualizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_froxelVisualizationRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_irraidanceInjectionSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_rayMarchingSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_rayMarchingSDC);

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_irraidanceInjectionResult);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_rayMarchingResult);

	return true;
}

bool VolumetricFogPass::froxelization()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	//auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMesh);
	//auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMaterial);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_froxelizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 3, 0, Accessibility::ReadWrite);

	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

	//auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

	//g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelizationRPDC, l_mesh);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.visibilityType == VisibilityType::Opaque)
		{
			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
			{
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
				g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

				g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelizationRPDC, l_drawCallData.mesh);
			}
		}
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 3, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_froxelizationRPDC);

	return true;
}

bool VolumetricFogPass::froxelVisualization()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_froxelVisualizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_froxelVisualizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_froxelVisualizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelVisualizationRPDC, ShaderStage::Vertex, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Quad);

	//for (size_t i = 0; i < 8 * 8 * 8 / 4; i++)
	{
		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelVisualizationRPDC, l_mesh);
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_froxelVisualizationRPDC, ShaderStage::Vertex, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_froxelVisualizationRPDC);

	return true;
}

bool VolumetricFogPass::irraidanceInjection()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = 64;
	auto l_numThreadsY = 64;
	auto l_numThreadsZ = 64;

	DispatchParamsConstantBuffer l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(8, 8, 8, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_irraidanceInjectionWorkload, 6, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_irraidanceInjectionRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 1, 6, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_irraidanceInjectionRPDC, 8, 8, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_irraidanceInjectionRPDC);

	return true;
}

bool VolumetricFogPass::rayMarching()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = 64;
	auto l_numThreadsY = 64;
	auto l_numThreadsZ = 64;

	DispatchParamsConstantBuffer l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(8, 8, 8, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_rayMarchingWorkload, 7, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_rayMarchingRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingSDC->m_ResourceBinder, 3, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 6, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_rayMarchingRPDC, 8, 8, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::PrepareCommandList()
{
	froxelization();
	froxelVisualization();
	irraidanceInjection();
	rayMarching();

	return true;
}

bool VolumetricFogPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_froxelVisualizationRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_froxelVisualizationRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->WaitForFrame(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_froxelVisualizationRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_rayMarchingRPDC);

	return true;
}

IResourceBinder * VolumetricFogPass::GetRayMarchingResult()
{
	return m_rayMarchingResult->m_ResourceBinder;
}

IResourceBinder * VolumetricFogPass::GetFroxelVisualizationResult()
{
	return m_froxelVisualizationRPDC->m_RenderTargets[0]->m_ResourceBinder;
}