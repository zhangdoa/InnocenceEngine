#include "DebugPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "GIDataLoader.h"
#include "OpaquePass.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

struct DebugMeshGPUData
{
	Mat4 m;
	uint32_t materialID;
	uint32_t padding[15];
};

struct DebugMaterialGPUData
{
	Vec4 color;
};

namespace DebugPass
{
	bool AddPDCMeshData(PhysicsDataComponent* PDC);
	bool AddBVHNode(BVHNode* node);

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

	m_debugSphereGPUData.reserve(m_maxDebugMeshes);
	m_debugCubeGPUData.reserve(m_maxDebugMeshes);
	m_debugMaterialGPUData.reserve(m_maxDebugMaterial);

	return true;
}

bool DebugPass::Initialize()
{
	return true;
}

bool DebugPass::AddPDCMeshData(PhysicsDataComponent* PDC)
{
	DebugMeshGPUData l_cubeMeshData;

	l_cubeMeshData.m = InnoMath::toTranslationMatrix(PDC->m_AABBWS.m_center);
	l_cubeMeshData.m.m00 *= PDC->m_AABBWS.m_extend.x / 2.0f;
	l_cubeMeshData.m.m11 *= PDC->m_AABBWS.m_extend.y / 2.0f;
	l_cubeMeshData.m.m22 *= PDC->m_AABBWS.m_extend.z / 2.0f;
	l_cubeMeshData.materialID = 4;

	m_debugCubeGPUData.emplace_back(l_cubeMeshData);

	DebugMeshGPUData l_sphereMeshData;

	l_sphereMeshData.m = InnoMath::toTranslationMatrix(PDC->m_SphereWS.m_center);
	l_sphereMeshData.m.m00 *= 0.5f;
	l_sphereMeshData.m.m11 *= 0.5f;
	l_sphereMeshData.m.m22 *= 0.5f;
	l_sphereMeshData.materialID = 3;

	m_debugSphereGPUData.emplace_back(l_sphereMeshData);

	return true;
}

bool DebugPass::AddBVHNode(BVHNode* node)
{
	if (node)
	{
		if (node->intermediatePDC)
		{
			AddPDCMeshData(node->intermediatePDC);
			if (node->leftChildNode)
			{
				AddBVHNode(node->leftChildNode);
			}
			if (node->rightChildNode)
			{
				AddBVHNode(node->rightChildNode);
			}
		}

		auto l_PDCCount = node->childrenPDCs.size();

		if (l_PDCCount)
		{
			for (size_t i = 0; i < l_PDCCount; i++)
			{
				auto l_PDC = node->childrenPDCs[i];
				AddPDCMeshData(l_PDC);
			}
		}
	}

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

		m_debugSphereGPUData.clear();
		m_debugCubeGPUData.clear();
		m_debugMaterialGPUData.clear();

		for (size_t i = 0; i < 5; i++)
		{
			m_debugMaterialGPUData.emplace_back();
		}

		m_debugMaterialGPUData[0].color = Vec4(0.1f, 0.2f, 0.4f, 1.0f);
		m_debugMaterialGPUData[1].color = Vec4(0.4f, 0.2f, 0.1f, 1.0f);
		m_debugMaterialGPUData[2].color = Vec4(0.9f, 0.1f, 0.0f, 1.0f);
		m_debugMaterialGPUData[3].color = Vec4(0.8f, 0.1f, 0.1f, 1.0f);
		m_debugMaterialGPUData[4].color = Vec4(0.1f, 0.6f, 0.2f, 1.0f);

		static bool l_drawProbesAndBricks = false;
		if (l_drawProbesAndBricks)
		{
			auto l_probes = GIDataLoader::GetProbes();

			if (l_probes.size() > 0)
			{
				for (size_t i = 0; i < l_probes.size(); i++)
				{
					DebugMeshGPUData l_meshData;

					l_meshData.m = InnoMath::toTranslationMatrix(l_probes[i].pos);
					l_meshData.m.m00 *= 0.5f;
					l_meshData.m.m11 *= 0.5f;
					l_meshData.m.m22 *= 0.5f;
					l_meshData.materialID = 0;

					m_debugSphereGPUData.emplace_back(l_meshData);
				}

				auto l_bricks = GIDataLoader::GetBricks();

				if (l_bricks.size() > 0)
				{
					for (size_t i = 0; i < l_bricks.size(); i++)
					{
						DebugMeshGPUData l_meshData;

						l_meshData.m = InnoMath::toTranslationMatrix(l_bricks[i].boundBox.m_center);
						l_meshData.m.m00 *= l_bricks[i].boundBox.m_extend.x / 2.0f;
						l_meshData.m.m11 *= l_bricks[i].boundBox.m_extend.y / 2.0f;
						l_meshData.m.m22 *= l_bricks[i].boundBox.m_extend.z / 2.0f;
						l_meshData.materialID = 1;

						m_debugCubeGPUData.emplace_back(l_meshData);
					}
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
			}
		}

		auto l_rootBVHNode = g_pModuleManager->getPhysicsSystem()->getRootBVHNode();

		AddBVHNode(l_rootBVHNode);

		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_debugMaterialGBDC, m_debugMaterialGPUData, 0, m_debugMaterialGPUData.size());
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_debugSphereMeshGBDC, m_debugSphereGPUData, 0, m_debugSphereGPUData.size());
		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_debugCubeMeshGBDC, m_debugCubeGPUData, 0, m_debugCubeGPUData.size());

		g_pModuleManager->getRenderingServer()->CommandListBegin(m_RPDC, 0);
		g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_RPDC);
		g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_RPDC);

		g_pModuleManager->getRenderingServer()->CopyDepthStencilBuffer(OpaquePass::GetRPDC(), m_RPDC);

		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, l_MainCameraGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);

		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Pixel, m_debugMaterialGBDC->m_ResourceBinder, 2, 1, Accessibility::ReadOnly);

		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, m_debugSphereMeshGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_sphere, m_debugSphereGPUData.size());

		g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_RPDC, ShaderStage::Vertex, m_debugCubeMeshGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
		g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_RPDC, l_cube, m_debugCubeGPUData.size());

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