#include "DebugPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "GIDataLoader.h"
#include "OpaquePass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

bool DebugPass::Setup(ISystemConfig *systemConfig)
{	
	auto l_cameraFrustumMeshCount = g_Engine->getRenderingFrontend()->getRenderingConfig().useCSM ? 4 : 1;
	m_debugCameraFrustumMeshComps.resize(l_cameraFrustumMeshCount);
	for (size_t i = 0; i < l_cameraFrustumMeshCount; i++)
	{
		m_debugCameraFrustumMeshComps[i] = g_Engine->getRenderingServer()->AddMeshComponent(("DebugCameraFrustumMesh_" + std::to_string(i) + "/").c_str());
		g_Engine->getAssetSystem()->generateProceduralMesh(ProceduralMeshShape::Cube, m_debugCameraFrustumMeshComps[i]);
		m_debugCameraFrustumMeshComps[i]->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
		m_debugCameraFrustumMeshComps[i]->m_ProceduralMeshShape = ProceduralMeshShape::Cube;
		m_debugCameraFrustumMeshComps[i]->m_ObjectStatus = ObjectStatus::Created;
	}
	
	m_debugSphereMeshGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("DebugSphereMeshGPUBuffer/");
	m_debugSphereMeshGPUBufferComp->m_ElementCount = m_maxDebugMeshes;
	m_debugSphereMeshGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugSphereMeshGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugCubeMeshGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("DebugCubeMeshGPUBuffer/");
	m_debugCubeMeshGPUBufferComp->m_ElementCount = m_maxDebugMeshes;
	m_debugCubeMeshGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugCubeMeshGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugCameraFrustumGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("DebugCameraFrustumGPUBuffer/");
	m_debugCameraFrustumGPUBufferComp->m_ElementCount = l_cameraFrustumMeshCount;
	m_debugCameraFrustumGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugCameraFrustumGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugMaterialGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("DebugMaterialGPUBuffer/");
	m_debugMaterialGPUBufferComp->m_ElementCount = m_maxDebugMaterial;
	m_debugMaterialGPUBufferComp->m_ElementSize = sizeof(DebugMaterialConstantBuffer);
	m_debugMaterialGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	////
	m_SPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("DebugPass/");

	m_SPC->m_ShaderFilePaths.m_VSPath = "debugPass.vert/";
	m_SPC->m_ShaderFilePaths.m_PSPath = "debugPass.frag/";
	m_RenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("DebugPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerFillMode = RasterizerFillMode::Wireframe;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ShaderProgram = m_SPC;

	m_debugSphereConstantBuffer.reserve(m_maxDebugMeshes);
	m_debugCubeConstantBuffer.reserve(m_maxDebugMeshes);
	m_debugMaterialConstantBuffer.reserve(m_maxDebugMaterial);

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool DebugPass::Initialize()
{	
	for (size_t i = 0; i < m_debugCameraFrustumMeshComps.size(); i++)
	{
		g_Engine->getRenderingServer()->InitializeMeshComponent(m_debugCameraFrustumMeshComps[i]);
	}

	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_debugSphereMeshGPUBufferComp);
	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_debugCubeMeshGPUBufferComp);
	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_debugCameraFrustumGPUBufferComp);
	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_debugMaterialGPUBufferComp);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_SPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DebugPass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus DebugPass::GetStatus()
{
	return m_ObjectStatus;
}

bool DebugPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingConfig = g_Engine->getRenderingFrontend()->getRenderingConfig();

	if (l_renderingConfig.drawDebugObject)
	{
		auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
		auto l_sphere = g_Engine->getRenderingFrontend()->getMeshComponent(ProceduralMeshShape::Sphere);
		auto l_cube = g_Engine->getRenderingFrontend()->getMeshComponent(ProceduralMeshShape::Cube);

		m_debugSphereConstantBuffer.clear();
		m_debugCubeConstantBuffer.clear();
		m_debugCameraFrustumConstantBuffer.clear();
		m_debugMaterialConstantBuffer.clear();

		for (size_t i = 0; i < 5; i++)
		{
			m_debugMaterialConstantBuffer.emplace_back();
		}

		m_debugMaterialConstantBuffer[0].color = Vec4(0.1f, 0.2f, 0.4f, 1.0f);
		m_debugMaterialConstantBuffer[1].color = Vec4(0.4f, 0.2f, 0.1f, 1.0f);
		m_debugMaterialConstantBuffer[2].color = Vec4(0.9f, 0.1f, 0.0f, 1.0f);
		m_debugMaterialConstantBuffer[3].color = Vec4(0.8f, 0.1f, 0.1f, 1.0f);
		m_debugMaterialConstantBuffer[4].color = Vec4(0.1f, 0.6f, 0.2f, 1.0f);

		static bool l_drawProbes = false;
		if (l_drawProbes)
		{
			auto l_probes = GIDataLoader::GetProbes();

			if (l_probes.size() > 0)
			{
				for (size_t i = 0; i < l_probes.size(); i++)
				{
					DebugPerObjectConstantBuffer l_meshData;

					l_meshData.m = InnoMath::toTranslationMatrix(l_probes[i].pos);
					l_meshData.m.m00 *= 0.5f;
					l_meshData.m.m11 *= 0.5f;
					l_meshData.m.m22 *= 0.5f;
					l_meshData.materialID = 0;

					m_debugSphereConstantBuffer.emplace_back(l_meshData);
				}

				auto l_brickFactor = GIDataLoader::GetBrickFactors();

				// @TODO:
				auto l_probeIndexBegin = 0;
				auto l_probeIndexEnd = 0;

				for (size_t probeIndex = l_probeIndexBegin; probeIndex < l_probeIndexEnd; probeIndex++)
				{
					m_debugSphereConstantBuffer[probeIndex].materialID = 2;

					auto l_probe = l_probes[probeIndex];

					for (size_t i = 0; i < 6; i++)
					{
						auto l_brickFactorBegin = l_probe.brickFactorRange[i * 2];
						auto l_brickFactorEnd = l_probe.brickFactorRange[i * 2 + 1];

						for (size_t j = l_brickFactorBegin; j < l_brickFactorEnd; j++)
						{
							auto l_brickIndex = l_brickFactor[j].brickIndex;
							m_debugCubeConstantBuffer[l_brickIndex].materialID = 3;
						}
					}
				}
			}
		}

		static bool l_drawBricks = false;
		if (l_drawBricks)
		{
			auto l_bricks = GIDataLoader::GetBricks();

			if (l_bricks.size() > 0)
			{
				for (size_t i = 0; i < l_bricks.size(); i++)
				{
					DebugPerObjectConstantBuffer l_meshData;

					l_meshData.m = InnoMath::toTranslationMatrix(l_bricks[i].boundBox.m_center);
					l_meshData.m.m00 *= l_bricks[i].boundBox.m_extend.x / 2.0f;
					l_meshData.m.m11 *= l_bricks[i].boundBox.m_extend.y / 2.0f;
					l_meshData.m.m22 *= l_bricks[i].boundBox.m_extend.z / 2.0f;
					l_meshData.materialID = 1;

					m_debugCubeConstantBuffer.emplace_back(l_meshData);
				}
			}
		}

		static bool l_drawBVHNodes = true;
		if (l_drawBVHNodes)
		{
			auto l_BVHNodes = g_Engine->getPhysicsSystem()->getBVHNodes();

			for (auto& i : l_BVHNodes)
			{
				AddBVHData(i);
			}
		}

		static bool l_drawCameraFrustums = true;
		if (l_drawCameraFrustums)
		{
			auto l_cameraComponent = static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->GetMainCamera();
			if(l_cameraComponent)
			{
				
				auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(l_cameraComponent->m_Owner);
				auto l_m = l_transformComponent->m_globalTransformMatrix.m_translationMat;
				for (size_t i = 0; i < m_debugCameraFrustumMeshComps.size(); i++)
				{
					std::vector<Vec4> l_vertices;
					std::vector<Index> l_indices =
					{
						0, 3, 1, 1, 3, 2,
						4, 0, 5, 5, 0, 1,
						7, 4, 6, 6, 4, 5,
						3, 7, 2, 2, 7, 6,
						4, 7, 0, 0, 7, 3,
						1, 2, 5, 5, 2, 6
					};

					l_vertices.resize(8);
					for (size_t j = 0; j < 8; j++)
					{
						l_vertices[j] = l_cameraComponent->m_splitFrustumVerticesWS[i * 8 + j].m_pos;
					}
					g_Engine->getAssetSystem()->fulfillVerticesAndIndices(m_debugCameraFrustumMeshComps[i], l_indices, l_vertices, 6);
					g_Engine->getRenderingServer()->UpdateMeshComponent(m_debugCameraFrustumMeshComps[i]);

					DebugPerObjectConstantBuffer l_meshData;
					l_meshData.m = InnoMath::generateIdentityMatrix<float>();
					l_meshData.materialID = 3;
					m_debugCameraFrustumConstantBuffer.emplace_back(l_meshData);
				}
				
				auto l_pCamera = l_cameraComponent->m_projectionMatrix;
				auto l_rCamera = InnoMath::toRotationMatrix(l_transformComponent->m_globalTransformVector.m_rot);
				auto l_tCamera = InnoMath::toTranslationMatrix(l_transformComponent->m_globalTransformVector.m_pos);

				auto l_vertices = InnoMath::generateFrustumVerticesWS(l_pCamera, l_rCamera, l_tCamera);

				for (auto& j : l_vertices)
				{
					DebugPerObjectConstantBuffer l_meshData;
					auto l_s = InnoMath::toScaleMatrix(Vec4(0.1f, 0.1f, 0.1f, 0.1f));
					l_meshData.m = InnoMath::toTranslationMatrix(j.m_pos);
					l_meshData.m = l_meshData.m * l_s;
					l_meshData.materialID = 4;
					m_debugSphereConstantBuffer.emplace_back(l_meshData);
				}
			}

			auto l_lightComponents = g_Engine->getComponentManager()->GetAll<LightComponent>();
			for (auto& i : l_lightComponents)
			{
				if(i->m_LightType == LightType::Directional)
				{
					for(auto& j : i->m_SplitAABBWS)
					{
						DebugPerObjectConstantBuffer l_meshData;
						auto l_s = InnoMath::toScaleMatrix(j.m_extend);
						l_meshData.m = InnoMath::toTranslationMatrix(j.m_center);
						l_meshData.m = l_meshData.m * l_s;
						l_meshData.materialID = 4;
						m_debugCubeConstantBuffer.emplace_back(l_meshData);
					}
				}
			}
		}

		static bool l_drawSkeletons = true;
		if (l_drawSkeletons)
		{
			auto l_visibleComponents = g_Engine->getComponentManager()->GetAll<VisibleComponent>();

			for (auto i : l_visibleComponents)
			{
				if (i->m_meshUsage == MeshUsage::Skeletal && i->m_model)
				{
					auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(i->m_Owner);
					auto l_m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

					for (size_t j = 0; j < i->m_model->meshMaterialPairs.m_count; j++)
					{
						auto l_pair = g_Engine->getAssetSystem()->getMeshMaterialPair(i->m_model->meshMaterialPairs.m_startOffset + j);
						auto l_skeleton = l_pair->mesh->m_SkeletonComp;

						for (auto k : l_skeleton->m_BoneData)
						{
							DebugPerObjectConstantBuffer l_meshData;
							auto l_bm = k.m_L2B;
							// Inverse-Joint-Matrix
							l_bm = l_bm.inverse();
							auto l_s = InnoMath::toScaleMatrix(Vec4(0.01f, 0.01f, 0.01f, 1.0f));
							l_bm = l_bm * l_s;
							l_m.m00 = 1.0f;
							l_m.m11 = 1.0f;
							l_m.m22 = 1.0f;
							l_bm = l_m * l_bm;

							l_meshData.m = l_bm;

							l_meshData.materialID = 2;

							m_debugCubeConstantBuffer.emplace_back(l_meshData);
						}
					}
				}
			}
		}

		g_Engine->getRenderingServer()->UploadGPUBufferComponent(m_debugMaterialGPUBufferComp, m_debugMaterialConstantBuffer, 0, m_debugMaterialConstantBuffer.size());
		if (m_debugSphereConstantBuffer.size())
		{
			g_Engine->getRenderingServer()->UploadGPUBufferComponent(m_debugSphereMeshGPUBufferComp, m_debugSphereConstantBuffer, 0, m_debugSphereConstantBuffer.size());
		}
		if (m_debugCubeConstantBuffer.size())
		{
			g_Engine->getRenderingServer()->UploadGPUBufferComponent(m_debugCubeMeshGPUBufferComp, m_debugCubeConstantBuffer, 0, m_debugCubeConstantBuffer.size());
		}
		if (m_debugCameraFrustumConstantBuffer.size())
		{
			g_Engine->getRenderingServer()->UploadGPUBufferComponent(m_debugCameraFrustumGPUBufferComp, m_debugCameraFrustumConstantBuffer, 0, m_debugCameraFrustumConstantBuffer.size());
		}

		g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
		g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
		g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);

		g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Pixel, m_debugMaterialGPUBufferComp, 2, Accessibility::ReadOnly);

		if (m_debugSphereConstantBuffer.size())
		{
			g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_debugSphereMeshGPUBufferComp, 1, Accessibility::ReadOnly);
			g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp, l_sphere, m_debugSphereConstantBuffer.size());
		}

		if (m_debugCubeConstantBuffer.size())
		{
			g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_debugCubeMeshGPUBufferComp, 1, Accessibility::ReadOnly);
			g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp, l_cube, m_debugCubeConstantBuffer.size());
		}

		if (m_debugCameraFrustumConstantBuffer.size())
		{
			for (size_t i = 0; i < m_debugCameraFrustumMeshComps.size(); i++)
			{
				g_Engine->getRenderingServer()->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_debugCameraFrustumGPUBufferComp, 1, Accessibility::ReadOnly, i, 1);
				g_Engine->getRenderingServer()->DrawIndexedInstanced(m_RenderPassComp, m_debugCameraFrustumMeshComps[i], 1);
			}
		}

		g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);
	}
	else
	{
		g_Engine->getRenderingServer()->CommandListBegin(m_RenderPassComp, 0);
		g_Engine->getRenderingServer()->BindRenderPassComponent(m_RenderPassComp);
		g_Engine->getRenderingServer()->CleanRenderTargets(m_RenderPassComp);

		g_Engine->getRenderingServer()->CommandListEnd(m_RenderPassComp);
	}

	return true;
}

RenderPassComponent* DebugPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

bool DebugPass::AddBVHData(const BVHNode& node)
{
	DebugPerObjectConstantBuffer l_cubeMeshData;

	l_cubeMeshData.m = InnoMath::toTranslationMatrix(node.m_AABB.m_center);
	l_cubeMeshData.m.m00 *= node.m_AABB.m_extend.x / 2.0f;
	l_cubeMeshData.m.m11 *= node.m_AABB.m_extend.y / 2.0f;
	l_cubeMeshData.m.m22 *= node.m_AABB.m_extend.z / 2.0f;
	l_cubeMeshData.materialID = node.PDC == nullptr ? 4 : 3;

	m_debugCubeConstantBuffer.emplace_back(l_cubeMeshData);

	return true;
}