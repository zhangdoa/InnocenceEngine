#include "VolumetricFogPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace VolumetricFogPass
{
	bool generateFroxel();
	bool rayMarching();

	RenderPassDataComponent* m_froxelRPDC;
	ShaderProgramComponent* m_froxelSPC;

	RenderPassDataComponent* m_rayMarchingRPDC;
	ShaderProgramComponent* m_rayMarchingSPC;

	TextureDataComponent* m_rayMarchingResult;
}

bool VolumetricFogPass::Setup()
{
	m_froxelSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogFroxelPass/");

	m_froxelSPC->m_ShaderFilePaths.m_VSPath = "volumetricFogFroxelPass.vert/";
	m_froxelSPC->m_ShaderFilePaths.m_GSPath = "volumetricFogFroxelPass.geom/";
	m_froxelSPC->m_ShaderFilePaths.m_PSPath = "volumetricFogFroxelPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_froxelSPC);

	m_froxelRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogFroxelPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler3D;
	//l_RenderPassDesc.m_RenderTargetDesc.UsageType = TextureUsageType::RawImage;
	//l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 160;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 90;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 64;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 160;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 90;

	m_froxelRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_froxelRPDC->m_ResourceBinderLayoutDescs.resize(4);
	m_froxelRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_froxelRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_froxelRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 2;

	m_froxelRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_froxelRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 3;

	//m_froxelRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	//m_froxelRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	//m_froxelRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	//m_froxelRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	//m_froxelRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 0;
	//m_froxelRPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_froxelRPDC->m_ShaderProgram = m_froxelSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_froxelRPDC);

	////
	m_rayMarchingSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VolumetricFogRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricFogRayMarchingPass.comp/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_rayMarchingSPC);

	m_rayMarchingRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VolumetricFogRayMarchingPass/");

	l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsageType = RenderPassUsageType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_rayMarchingRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs.resize(5);
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 7;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 8;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_BinderAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 0;
	m_rayMarchingRPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_rayMarchingRPDC->m_ShaderProgram = m_rayMarchingSPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_rayMarchingRPDC);

	////
	m_rayMarchingResult = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("RayMarchingResult/");
	m_rayMarchingResult->m_textureDataDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_rayMarchingResult->m_textureDataDesc.UsageType = TextureUsageType::RawImage;
	m_rayMarchingResult->m_textureDataDesc.MinFilterMethod = TextureFilterMethod::Linear;
	m_rayMarchingResult->m_textureDataDesc.MagFilterMethod = TextureFilterMethod::Linear;

	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_rayMarchingResult);

	return true;
}

bool VolumetricFogPass::Initialize()
{
	return true;
}

bool VolumetricFogPass::generateFroxel()
{
	auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
	//auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMesh);
	//auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::VolumetricFogPassMaterial);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::OpaquePassMesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::OpaquePassMaterial);
	auto l_SunGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Sun);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_froxelRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_froxelRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_froxelRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelRPDC, ShaderStage::Vertex, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelRPDC, ShaderStage::Pixel, l_SunGBDC->m_ResourceBinder, 3, 3, Accessibility::ReadOnly);
	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelRPDC, ShaderStage::Pixel, m_froxelRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadWrite);

	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshGPUDataIndex, 1);
	//g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialGPUDataIndex, 1);

	//auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

	//g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelRPDC, l_mesh);

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	auto& l_opaquePassDrawCallData = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallData();

	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_drawCallData = l_opaquePassDrawCallData[i];
		if (l_drawCallData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_froxelRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_offset, 1);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_froxelRPDC, l_drawCallData.mesh);
		}

		l_offset++;
	}

	//g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_froxelRPDC, ShaderStage::Pixel, m_froxelRPDC->m_RenderTargetsResourceBinders[0], 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_froxelRPDC);

	return true;
}
bool VolumetricFogPass::rayMarching()
{
	auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Compute);

	auto l_numThreadsX = 1280;
	auto l_numThreadsY = 720;
	auto l_numThreadsZ = 64;

	DispatchParamsGPUData l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<unsigned int>(160, 90, 8, 0);
	l_rayMarchingWorkload.numThreads = TVec4<unsigned int>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_rayMarchingWorkload, 6, 1);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_rayMarchingRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_rayMarchingRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, l_dispatchParamsGBDC->m_ResourceBinder, 2, 8, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_froxelRPDC->m_RenderTargetsResourceBinders[0], 3, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_rayMarchingRPDC, 160, 90, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_froxelRPDC->m_RenderTargetsResourceBinders[0], 3, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayMarchingRPDC, ShaderStage::Compute, m_rayMarchingResult->m_ResourceBinder, 4, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::PrepareCommandList()
{
	generateFroxel();
	rayMarching();

	return true;
}

bool VolumetricFogPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_froxelRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_froxelRPDC);

	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_rayMarchingRPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_rayMarchingRPDC);

	return true;
}

bool VolumetricFogPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_froxelRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_rayMarchingRPDC);

	return true;
}

IResourceBinder * VolumetricFogPass::GetRayMarchingResult()
{
	return m_rayMarchingResult->m_ResourceBinder;
}