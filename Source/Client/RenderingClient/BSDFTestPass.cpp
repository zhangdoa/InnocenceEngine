#include "BSDFTestPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "BRDFLUTPass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace BSDFTestPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
	SamplerDataComponent* m_SDC;

	std::vector<MeshGPUData> m_meshGPUData;
	std::vector<MaterialGPUData> m_materialGPUData;

	const size_t m_shpereCount = 10;
}

bool BSDFTestPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("BSDFTestPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "opaquePass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "BSDFTestPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("BSDFTestPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor[0] = 1.0f;
	l_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor[1] = 1.0f;
	l_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor[2] = 1.0f;
	l_RenderPassDesc.m_GraphicsPipelineDesc.CleanColor[3] = 1.0f;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(6);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 1;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 2;

	m_RPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_GlobalSlot = 3;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_GlobalSlot = 4;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_LocalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[4].m_IsRanged = true;

	m_RPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_GlobalSlot = 5;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[5].m_IsRanged = true;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_SDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("BSDFTestPass/");

	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_SDC);

	//
	auto l_RenderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_meshGPUData.resize(l_RenderingCapability.maxMeshes);
	m_materialGPUData.resize(l_RenderingCapability.maxMaterials);

	size_t l_index = 0;

	auto l_interval = 4.0f;

	for (size_t i = 0; i < m_shpereCount; i++)
	{
		for (size_t j = 0; j < m_shpereCount; j++)
		{
			MeshGPUData l_meshGPUData;
			l_meshGPUData.m = InnoMath::toTranslationMatrix(Vec4((float)i * l_interval, 0.0f, (float)j * l_interval, 1.0f));
			l_meshGPUData.normalMat = InnoMath::generateIdentityMatrix<float>();

			m_meshGPUData[l_index] = l_meshGPUData;

			MaterialGPUData l_materialGPUData;

			l_materialGPUData.customMaterial.Metallic = (float)i / (float)m_shpereCount;
			l_materialGPUData.customMaterial.Roughness = (float)j / (float)m_shpereCount;

			m_materialGPUData[l_index] = l_materialGPUData;

			l_index++;
		}
	}

	return true;
}

bool BSDFTestPass::Initialize()
{
	return true;
}

bool BSDFTestPass::PrepareCommandList()
{
	auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::OpaquePassMesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::OpaquePassMaterial);

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MeshGBDC, m_meshGPUData);
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_MaterialGBDC, m_materialGPUData);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_SDC->m_ResourceBinder, 5, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 3, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 4, 1);

	auto l_mesh = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Sphere);

	uint32_t l_offset = 0;

	for (size_t i = 0; i < m_shpereCount * m_shpereCount; i++)
	{
		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_offset, 1);
		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_offset, 1);
		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_mesh);

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFLUT(), 3, 0);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_RPDC, ShaderStage::Pixel, BRDFLUTPass::GetBRDFMSLUT(), 4, 1);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool BSDFTestPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool BSDFTestPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent * BSDFTestPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * BSDFTestPass::GetSPC()
{
	return m_SPC;
}