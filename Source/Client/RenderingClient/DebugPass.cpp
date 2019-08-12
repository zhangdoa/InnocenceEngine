#include "DebugPass.h"
#include "DefaultGPUBuffers.h"

#include "GIBakePass.h"
#include "OpaquePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace DebugPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;

	std::vector<mat4> m_debugSphereGPUData;
	std::vector<mat4> m_debugCubeGPUData;
}

bool DebugPass::Setup()
{
	m_SPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("DebugPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "debugPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "debugPass.frag/";

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);

	m_RPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("DebugPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseDepthBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_UseStencilBuffer = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowStencilWrite = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_FrontFaceStencilComparisionFunction = ComparisionFunction::Always;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_BackFaceStencilComparisionFunction = ComparisionFunction::Always;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerFillMode = RasterizerFillMode::Wireframe;

	m_RPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_RPDC->m_ResourceBinderLayoutDescs.resize(2);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 13;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	auto l_RenderingCapability = g_pModuleManager->getRenderingFrontend()->getRenderingCapability();

	m_debugSphereGPUData.resize(l_RenderingCapability.maxMeshes);
	m_debugCubeGPUData.resize(l_RenderingCapability.maxMeshes);

	return true;
}

bool DebugPass::Initialize()
{
	return true;
}

bool DebugPass::PrepareCommandList()
{
	auto l_renderingConfig = g_pModuleManager->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.drawDebugObject)
	{
		auto l_CameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Camera);
		auto l_DebugGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Debug);

		auto l_probes = GIBakePass::GetProbes();
		if (l_probes.size() > 0)
		{
			for (size_t i = 0; i < l_probes.size(); i++)
			{
				m_debugSphereGPUData[i] = InnoMath::toTranslationMatrix(l_probes[i].pos);
				m_debugSphereGPUData[i].m00 *= 0.5f;
				m_debugSphereGPUData[i].m11 *= 0.5f;
				m_debugSphereGPUData[i].m22 *= 0.5f;
			}

			auto l_bricks = GIBakePass::GetBricks();
			for (size_t i = 0; i < l_bricks.size(); i++)
			{
				m_debugCubeGPUData[i] = InnoMath::toTranslationMatrix(l_bricks[i].boundBox.m_center);
				m_debugCubeGPUData[i].m00 *= l_bricks[i].boundBox.m_extend.x / 2.0f;
				m_debugCubeGPUData[i].m11 *= l_bricks[i].boundBox.m_extend.y / 2.0f;
				m_debugCubeGPUData[i].m22 *= l_bricks[i].boundBox.m_extend.z / 2.0f;
			}

			g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_DebugGBDC, m_debugSphereGPUData, 0, l_probes.size());
			g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(l_DebugGBDC, m_debugCubeGPUData, l_probes.size(), l_bricks.size());

			g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
			g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
			g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

			g_pModuleManager->getRenderingServer()->CopyDepthBuffer(OpaquePass::GetRPDC(), m_RPDC);

			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_CameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);

			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_DebugGBDC->m_ResourceBinder, 1, 13, Accessibility::ReadOnly, 0, l_probes.size());
			auto l_sphere = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Sphere);
			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_sphere, l_probes.size());

			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_DebugGBDC->m_ResourceBinder, 1, 13, Accessibility::ReadOnly, l_probes.size(), l_bricks.size());
			auto l_cube = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);
			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_cube, l_bricks.size());
		}

		g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);
	}
	else
	{
		g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
		g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
		g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

		g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);
	}

	return true;
}

bool DebugPass::ExecuteCommandList()
{
	g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_RPDC);

	g_pModuleManager->getRenderingServer()->WaitForFrame(m_RPDC);

	return true;
}

bool DebugPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_RPDC);

	return true;
}

RenderPassDataComponent * DebugPass::GetRPDC()
{
	return m_RPDC;
}

ShaderProgramComponent * DebugPass::GetSPC()
{
	return m_SPC;
}