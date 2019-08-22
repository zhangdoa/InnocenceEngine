#include "DebugPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "GIDataLoader.h"
#include "OpaquePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

struct DebugMeshGPUData
{
	mat4 m;
	unsigned int materialID;
	unsigned int padding[15];
};

struct DebugMaterialGPUData
{
	vec4 color;
};

namespace DebugPass
{
	RenderPassDataComponent* m_RPDC;
	ShaderProgramComponent* m_SPC;

	GPUBufferDataComponent* m_debugSphereMeshGBDC;
	GPUBufferDataComponent* m_debugCubeMeshGBDC;
	GPUBufferDataComponent* m_debugMaterialGBDC;

	const size_t m_maxDebugMeshes = 65536;
	const size_t m_maxDebugMaterial = 512;
	std::vector<DebugMeshGPUData> m_debugSphereGPUData;
	std::vector<DebugMeshGPUData> m_debugCubeGPUData;
	std::vector<DebugMaterialGPUData> m_debugMaterialGPUData;
}

bool DebugPass::Setup()
{
	m_debugSphereMeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("DebugSphereMeshGPUBuffer/");
	m_debugSphereMeshGBDC->m_ElementCount = m_maxDebugMeshes;
	m_debugSphereMeshGBDC->m_ElementSize = sizeof(DebugMeshGPUData);
	m_debugSphereMeshGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_debugSphereMeshGBDC->m_BindingPoint = 0;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_debugSphereMeshGBDC);

	m_debugCubeMeshGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("DebugCubeMeshGPUBuffer/");
	m_debugCubeMeshGBDC->m_ElementCount = m_maxDebugMeshes;
	m_debugCubeMeshGBDC->m_ElementSize = sizeof(DebugMeshGPUData);
	m_debugCubeMeshGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_debugCubeMeshGBDC->m_BindingPoint = 0;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_debugCubeMeshGBDC);

	m_debugMaterialGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("DebugMaterialGPUBuffer/");
	m_debugMaterialGBDC->m_ElementCount = m_maxDebugMaterial;
	m_debugMaterialGBDC->m_ElementSize = sizeof(DebugMaterialGPUData);
	m_debugMaterialGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_debugMaterialGBDC->m_BindingPoint = 1;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_debugMaterialGBDC);

	////
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

	m_RPDC->m_ResourceBinderLayoutDescs.resize(3);
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_GlobalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[0].m_LocalSlot = 0;

	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_GlobalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_LocalSlot = 0;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_GlobalSlot = 2;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_LocalSlot = 1;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadOnly;
	m_RPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RPDC->m_ShaderProgram = m_SPC;

	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_RPDC);

	m_debugSphereGPUData.resize(m_maxDebugMeshes);
	m_debugCubeGPUData.resize(m_maxDebugMeshes);
	m_debugMaterialGPUData.resize(m_maxDebugMaterial);

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
		auto l_MainCameraGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::MainCamera);
		auto l_sphere = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Sphere);
		auto l_cube = g_pModuleManager->getRenderingFrontend()->getMeshDataComponent(MeshShapeType::Cube);

		m_debugMaterialGPUData[0].color = vec4(0.1f, 0.2f, 0.4f, 1.0f);
		m_debugMaterialGPUData[1].color = vec4(0.4f, 0.2f, 0.1f, 1.0f);
		m_debugMaterialGPUData[2].color = vec4(0.9f, 0.1f, 0.0f, 1.0f);
		m_debugMaterialGPUData[3].color = vec4(0.8f, 0.1f, 0.1f, 1.0f);

		auto l_probes = GIDataLoader::GetProbes();

		if (l_probes.size() > 0)
		{
			for (size_t i = 0; i < l_probes.size(); i++)
			{
				m_debugSphereGPUData[i].m = InnoMath::toTranslationMatrix(l_probes[i].pos);
				m_debugSphereGPUData[i].m.m00 *= 0.5f;
				m_debugSphereGPUData[i].m.m11 *= 0.5f;
				m_debugSphereGPUData[i].m.m22 *= 0.5f;
				m_debugSphereGPUData[i].materialID = 0;
			}

			auto l_bricks = GIDataLoader::GetBricks();
			for (size_t i = 0; i < l_bricks.size(); i++)
			{
				m_debugCubeGPUData[i].m = InnoMath::toTranslationMatrix(l_bricks[i].boundBox.m_center);
				m_debugCubeGPUData[i].m.m00 *= l_bricks[i].boundBox.m_extend.x / 2.0f;
				m_debugCubeGPUData[i].m.m11 *= l_bricks[i].boundBox.m_extend.y / 2.0f;
				m_debugCubeGPUData[i].m.m22 *= l_bricks[i].boundBox.m_extend.z / 2.0f;
				m_debugCubeGPUData[i].materialID = 1;
			}

			auto l_brickFactor = GIDataLoader::GetBrickFactors();

			// @TODO:
			auto l_probeIndexBegin = 15;
			auto l_probeIndexEnd = 16;

			for (size_t probeIndex = l_probeIndexBegin; probeIndex < l_probeIndexEnd; probeIndex++)
			{
				m_debugSphereGPUData[probeIndex].materialID = 2;

				auto l_probe = l_probes[probeIndex];

				for (size_t i = 0; i < 6; i++)
				{
					auto l_brickFactorBegin = l_probe.brickFactorRange[i * 2];
					auto l_brickFactorEnd = l_probe.brickFactorRange[i * 2 + 1];

					for (size_t j = l_brickFactorBegin; j < l_brickFactorEnd; j++)
					{
						auto l_brickIndex = l_brickFactor[j].brickIndex;
						m_debugCubeGPUData[l_brickIndex].materialID = 3;
					}
				}
			}

			g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_debugMaterialGBDC, m_debugMaterialGPUData, 0, 4);
			g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_debugSphereMeshGBDC, m_debugSphereGPUData, 0, l_probes.size());
			g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_debugCubeMeshGBDC, m_debugCubeGPUData, 0, l_bricks.size());

			g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
			g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
			g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

			g_pModuleManager->getRenderingServer()->CopyDepthBuffer(OpaquePass::GetRPDC(), m_RPDC);

			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);

			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_debugMaterialGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadOnly);

			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, m_debugSphereMeshGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_sphere, l_probes.size());

			g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, m_debugCubeMeshGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);

			g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_cube, l_bricks.size());

			g_pModuleManager->getRenderingServer()->CommandListEnd(m_RPDC);
		}
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