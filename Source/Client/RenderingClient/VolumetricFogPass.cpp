#include "VolumetricFogPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace VolumetricFogPass
{
	bool froxelization();
	bool irraidanceInjection();
	bool rayMarching();

	RenderPassDataComponent* m_froxelizationRPDC;
	ShaderProgramComponent* m_froxelizationSPC;

	RenderPassDataComponent* m_irraidanceInjectionRPDC;
	ShaderProgramComponent* m_irraidanceInjectionSPC;

	RenderPassDataComponent* m_rayMarchingRPDC;
	ShaderProgramComponent* m_rayMarchingSPC;
	SamplerDataComponent* m_rayMarchingSDC;

	TextureDataComponent* m_irraidanceInjectionResult;
	TextureDataComponent* m_rayMarchingResult;
}

bool VolumetricFogPass::Setup()
{
	m_froxelizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogFroxelizationPass/");

	m_froxelizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricFogFroxelizationPass.vert/";
	m_froxelizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricFogFroxelizationPass.geom/";
	m_froxelizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricFogFroxelizationPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_froxelizationSPC);

	m_froxelizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogFroxelizationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::RawImage;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 160;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 90;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 64;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 160;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 90;

	m_froxelizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 2;

	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_froxelizationRPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_froxelizationRPDC->m_ShaderProgram = m_froxelizationSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_froxelizationRPDC);

	////
	m_irraidanceInjectionSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogIrraidanceInjectionPass/");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricFogIrraidanceInjectionPass.comp/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_irraidanceInjectionSPC);

	l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_irraidanceInjectionRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogIrraidanceInjectionPass/");

	m_irraidanceInjectionRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs.resize(6);
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 3;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 7;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 8;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 0;
	m_irraidanceInjectionRPDC->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_irraidanceInjectionRPDC->m_ShaderProgram = m_irraidanceInjectionSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_irraidanceInjectionRPDC);

	////
	m_rayMarchingSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricFogRayMarchingPass.comp/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_rayMarchingSPC);

	m_rayMarchingRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogRayMarchingPass/");

	m_rayMarchingRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 8;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_IsRanged = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_IsRanged = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_rayMarchingRPDC->m_ShaderProgram = m_rayMarchingSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_rayMarchingRPDC);

	m_rayMarchingSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VolumetricFogRayMarchingPass/");

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_rayMarchingSDC);
	////
	m_irraidanceInjectionResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricFogIrraidanceInjectionResult/");
	m_irraidanceInjectionResult->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_irraidanceInjectionResult->m_textureDataDesc.SamplerType = TextureSamplerType::Sampler3D;
	m_irraidanceInjectionResult->m_textureDataDesc.UsageType = TextureUsageType::RawImage;
	m_irraidanceInjectionResult->m_textureDataDesc.GPUAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionResult->m_textureDataDesc.MinFilterMethod = TextureFilterMethod::Linear;
	m_irraidanceInjectionResult->m_textureDataDesc.MagFilterMethod = TextureFilterMethod::Linear;
	m_irraidanceInjectionResult->m_textureDataDesc.Width = 160;
	m_irraidanceInjectionResult->m_textureDataDesc.Height = 90;
	m_irraidanceInjectionResult->m_textureDataDesc.DepthOrArraySize = 64;

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_irraidanceInjectionResult);

	m_rayMarchingResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VolumetricFogRayMarchingResult/");
	m_rayMarchingResult->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_rayMarchingResult->m_textureDataDesc.UsageType = TextureUsageType::RawImage;
	m_rayMarchingResult->m_textureDataDesc.GPUAccessibility = Accessibility::ReadWrite;

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_rayMarchingResult);

	return true;
}

bool VolumetricFogPass::Initialize()
{
	return true;
}

bool VolumetricFogPass::froxelization()
{
	auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
	//auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMesh);
	//auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMaterial);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::OpaquePassMesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::OpaquePassMaterial);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_froxelizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 3, 0, Accessibility::ReadWrite);

	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshGPUDataIndex, 1);
	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialGPUDataIndex, 1);

	//auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

	//g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelizationRPDC, l_mesh);

	uint32_t l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	auto& l_opaquePassDrawCallData = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallData();

	for (uint32_t i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_drawCallData = l_opaquePassDrawCallData[i];
		if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_offset, 1);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelizationRPDC, l_drawCallData.mesh);
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_froxelizationRPDC);

	return true;
}
bool VolumetricFogPass::irraidanceInjection()
{
	auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);
	auto l_SkyGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sky);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);

	auto l_numThreadsX = 160;
	auto l_numThreadsY = 90;
	auto l_numThreadsZ = 64;

	DispatchParamsGPUData l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(20, 12, 8, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_irraidanceInjectionWorkload, 6, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_irraidanceInjectionRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_SunGBDC->m_ResourceBinder, 1, 3, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_SkyGBDC->m_ResourceBinder, 2, 7, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 3, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 5, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_irraidanceInjectionRPDC, 20, 12, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 5, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_irraidanceInjectionRPDC);

	return true;
}

bool VolumetricFogPass::rayMarching()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);

	auto l_numThreadsX = 1280;
	auto l_numThreadsY = 720;
	auto l_numThreadsZ = 1;

	DispatchParamsGPUData l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(160, 90, 1, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_rayMarchingWorkload, 7, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_rayMarchingRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingSDC->m_ResourceBinder, 3, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 0, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_rayMarchingRPDC, 160, 90, 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::PrepareCommandList()
{
	froxelization();
	irraidanceInjection();
	rayMarching();

	return true;
}

bool VolumetricFogPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_froxelizationRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_irraidanceInjectionRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_froxelizationRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_irraidanceInjectionRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_rayMarchingRPDC);

	return true;
}

IResourceBinder * VolumetricFogPass::GetRayMarchingResult()
{
	return m_rayMarchingResult->m_ResourceBinder;
}