#include "OpaquePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace OpaquePass
{
	RenderPassDataComponent* m_RPC;
	ShaderProgramComponent* m_SPC;

	GPUBufferDataComponent* m_CameraGBDC;
	GPUBufferDataComponent* m_MeshGBDC;
	GPUBufferDataComponent* m_MaterialGBDC;
}

bool OpaquePass::Initialize()
{
	m_RPC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("OpaquePass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 4;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilReference = 0x01;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilPassOperation = StencilOperation::Replace;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilPassOperation = StencilOperation::Replace;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;

	m_RPC->m_RenderPassDesc = l_RenderPassDesc;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPC);

	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("OpaquePass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "opaquePass.vert/";
	m_SPC->m_ShaderFilePaths.m_FSPath = "opaquePass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	auto l_RenderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_CameraGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("CameraGPUBuffer/");
	m_CameraGBDC->m_Size = sizeof(CameraGPUData);
	m_CameraGBDC->m_BindingPoint = 0;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_CameraGBDC);

	m_MeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("MeshGPUBuffer/");
	m_MeshGBDC->m_Size = l_RenderingCapability.maxMeshes * sizeof(MeshGPUData);
	m_MeshGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_MeshGBDC);

	m_MaterialGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("MaterialGPUBuffer/");
	m_MaterialGBDC->m_Size = l_RenderingCapability.maxMaterials * sizeof(MaterialGPUData);
	m_MaterialGBDC->m_BindingPoint = 2;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_MaterialGBDC);

	return true;
}

bool OpaquePass::PrepareCommandList()
{
	auto l_cameraGPUData = g_pModuleManager->getRenderingFrontend()->getCameraGPUData();
	auto l_meshGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassMeshGPUData();
	auto l_materialGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassMaterialGPUData();

	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_CameraGBDC, &l_cameraGPUData);
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_MeshGBDC, l_meshGPUData);
	g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_MaterialGBDC, l_materialGPUData);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPC);
	g_pModuleManager->getRenderingServer()->BindShaderProgramComponent(m_SPC);

	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::VERTEX, GPUBufferAccessibility::ReadOnly, m_CameraGBDC, 0, sizeof(CameraGPUData));

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_opaquePassGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassGPUData()[i];

		g_pModuleManager->getRenderingServer()->BindMaterialDataComponent(l_opaquePassGPUData.material);

		g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::VERTEX, GPUBufferAccessibility::ReadOnly, m_MeshGBDC, l_offset * sizeof(MeshGPUData), sizeof(MeshGPUData));
		g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, m_MaterialGBDC, l_offset * sizeof(MaterialGPUData), sizeof(MaterialGPUData));

		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPC, l_opaquePassGPUData.mesh);

		g_pModuleManager->getRenderingServer()->UnbindMaterialDataComponent(l_opaquePassGPUData.material);

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPC, 0);

	return true;
}

RenderPassDataComponent * OpaquePass::getRPC()
{
	return m_RPC;
}

ShaderProgramComponent * OpaquePass::getSPC()
{
	return m_SPC;
}