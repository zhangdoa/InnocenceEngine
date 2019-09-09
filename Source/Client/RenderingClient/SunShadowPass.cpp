#include "SunShadowPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace SunShadowPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;
}

bool SunShadowPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("SunShadowPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "sunShadowPass.vert/";
	m_SPC->m_ShaderFilePaths.m_GSPath = "sunShadowPass.geom/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "sunShadowPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("SunShadowPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_RenderTargetDesc.SamplerType = TextureSamplerType::Sampler2DArray;
	l_RenderPassDesc.m_RenderTargetDesc.Width = 2048;
	l_RenderPassDesc.m_RenderTargetDesc.Height = 2048;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = 4;
	l_RenderPassDesc.m_RenderTargetDesc.WrapMethod = TextureWrapMethod::Border;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::FLOAT32;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[0] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[1] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[2] = 1.0f;
	l_RenderPassDesc.m_RenderTargetDesc.BorderColor[3] = 1.0f;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = 2048;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = 2048;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerCullMode = RasterizerCullMode::Front;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 1;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 6;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	return true;
}

bool SunShadowPass::Initialize()
{
	return true;
}

bool SunShadowPass::PrepareCommandList()
{
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::SunShadowPassMesh);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Geometry, l_CSMGBDC->m_ResourceBinder, 1, 6, Accessibility::ReadOnly);

	uint32_t l_offset = 0;

	auto l_totalDrawCallCount = g_pModuleManager->getRenderingFrontend()->getSunShadowPassDrawCallCount();
	auto l_sunShadowPassDrawCallData = g_pModuleManager->getRenderingFrontend()->getSunShadowPassDrawCallData();

	for (uint32_t i = 0; i < l_totalDrawCallCount; i++)
	{
		auto l_drawCallData = l_sunShadowPassDrawCallData[i];

		if (l_drawCallData.mesh->m_objectStatus == ObjectStatus::Activated)
		{
			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 0, 1, Accessibility::ReadOnly, l_offset, 1);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_drawCallData.mesh);
		}

		l_offset++;
	}

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);

	return true;
}

bool SunShadowPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool SunShadowPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent * SunShadowPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * SunShadowPass::GetSPC()
{
	return m_SPC;
}