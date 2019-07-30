#include "OpaquePass.h"
#include "DefaultGPUBuffers.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace OpaquePass
{
	RenderPassDataComponent* m_RPC;
	ShaderProgramComponent* m_SPC;
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

	m_RPC->m_ResourceBinderLayoutDescs.resize(5);
	m_RPC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::ROBuffer;
	m_RPC->m_ResourceBinderLayoutDescs[0].m_BindingSlot = 0;

	m_RPC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPC->m_ResourceBinderLayoutDescs[1].m_BindingSlot = 1;

	m_RPC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::ROBufferArray;
	m_RPC->m_ResourceBinderLayoutDescs[2].m_BindingSlot = 2;

	m_RPC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Image;
	m_RPC->m_ResourceBinderLayoutDescs[3].m_BindingSlot = 3;
	m_RPC->m_ResourceBinderLayoutDescs[3].m_ResourceCount = 5;
	m_RPC->m_ResourceBinderLayoutDescs[3].m_IsRanged = true;

	m_RPC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_RPC->m_ResourceBinderLayoutDescs[4].m_BindingSlot = 4;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPC);

	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("OpaquePass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "opaquePass.vert/";
	m_SPC->m_ShaderFilePaths.m_FSPath = "opaquePass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	return true;
}

bool OpaquePass::PrepareCommandList()
{
	auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPC);
	g_pModuleManager->getRenderingServer()->BindShaderProgramComponent(m_SPC);

	g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::VERTEX, GPUBufferAccessibility::ReadOnly, l_CameraGBDC, 0, l_CameraGBDC->m_TotalSize);

	unsigned int l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getOpaquePassDrawCallCount();
	for (unsigned int i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_opaquePassGPUData = g_pModuleManager->getRenderingFrontend()->getOpaquePassGPUData()[i];

		g_pModuleManager->getRenderingServer()->BindMaterialDataComponent(ShaderType::FRAGMENT, l_opaquePassGPUData.material);

		g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::VERTEX, GPUBufferAccessibility::ReadOnly, l_MeshGBDC, l_offset, l_MeshGBDC->m_ElementSize);
		g_pModuleManager->getRenderingServer()->BindGPUBufferDataComponent(ShaderType::FRAGMENT, GPUBufferAccessibility::ReadOnly, l_MaterialGBDC, l_offset, l_MaterialGBDC->m_ElementSize);

		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPC, l_opaquePassGPUData.mesh);

		g_pModuleManager->getRenderingServer()->UnbindMaterialDataComponent(ShaderType::FRAGMENT, l_opaquePassGPUData.material);

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