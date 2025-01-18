#include "DebugPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Services/TemplateAssetService.h"
#include "../../Engine/Services/AssetSystem.h"
#include "../../Engine/Services/ComponentManager.h"

#include "GIDataLoader.h"
#include "OpaquePass.h"

#include "../../Engine/Engine.h"

using namespace Inno;


using namespace DefaultGPUBuffers;

bool DebugPass::Setup(ISystemConfig *systemConfig)
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	
	auto l_cameraFrustumMeshCount = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig().useCSM ? 4 : 1;
	m_debugCameraFrustumMeshComps.resize(l_cameraFrustumMeshCount);
	for (size_t i = 0; i < l_cameraFrustumMeshCount; i++)
	{
		m_debugCameraFrustumMeshComps[i] = l_renderingServer->AddMeshComponent(("DebugCameraFrustumMesh_" + std::to_string(i) + "/").c_str());
		g_Engine->Get<AssetSystem>()->GenerateProceduralMesh(ProceduralMeshShape::Cube, m_debugCameraFrustumMeshComps[i]);
		m_debugCameraFrustumMeshComps[i]->m_MeshPrimitiveTopology = MeshPrimitiveTopology::Triangle;
		m_debugCameraFrustumMeshComps[i]->m_ProceduralMeshShape = ProceduralMeshShape::Cube;
		m_debugCameraFrustumMeshComps[i]->m_ObjectStatus = ObjectStatus::Created;
	}
	
	m_debugSphereMeshGPUBufferComp = l_renderingServer->AddGPUBufferComponent("DebugSphereMeshGPUBuffer/");
	m_debugSphereMeshGPUBufferComp->m_ElementCount = m_maxDebugMeshes;
	m_debugSphereMeshGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugSphereMeshGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugCubeMeshGPUBufferComp = l_renderingServer->AddGPUBufferComponent("DebugCubeMeshGPUBuffer/");
	m_debugCubeMeshGPUBufferComp->m_ElementCount = m_maxDebugMeshes;
	m_debugCubeMeshGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugCubeMeshGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugCameraFrustumGPUBufferComp = l_renderingServer->AddGPUBufferComponent("DebugCameraFrustumGPUBuffer/");
	m_debugCameraFrustumGPUBufferComp->m_ElementCount = l_cameraFrustumMeshCount;
	m_debugCameraFrustumGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugCameraFrustumGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugMaterialGPUBufferComp = l_renderingServer->AddGPUBufferComponent("DebugMaterialGPUBuffer/");
	m_debugMaterialGPUBufferComp->m_ElementCount = m_maxDebugMaterial;
	m_debugMaterialGPUBufferComp->m_ElementSize = sizeof(DebugMaterialConstantBuffer);
	m_debugMaterialGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	////
	m_ShaderProgramComp = l_renderingServer->AddShaderProgramComponent("DebugPass/");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "debugPass.vert/";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "debugPass.frag/";
	m_RenderPassComp = l_renderingServer->AddRenderPassComponent("DebugPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

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

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_RenderPassComp->m_OnResize = std::bind(&DebugPass::InitializeResourceBindingLayoutDescs, this);

	m_debugSphereConstantBuffer.reserve(m_maxDebugMeshes);
	m_debugCubeConstantBuffer.reserve(m_maxDebugMeshes);
	m_debugMaterialConstantBuffer.reserve(m_maxDebugMaterial);

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool DebugPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	for (size_t i = 0; i < m_debugCameraFrustumMeshComps.size(); i++)
	{
		l_renderingServer->InitializeMeshComponent(m_debugCameraFrustumMeshComps[i]);
	}

	l_renderingServer->InitializeGPUBufferComponent(m_debugSphereMeshGPUBufferComp);
	l_renderingServer->InitializeGPUBufferComponent(m_debugCubeMeshGPUBufferComp);
	l_renderingServer->InitializeGPUBufferComponent(m_debugCameraFrustumGPUBufferComp);
	l_renderingServer->InitializeGPUBufferComponent(m_debugMaterialGPUBufferComp);

	l_renderingServer->InitializeShaderProgramComponent(m_ShaderProgramComp);
	l_renderingServer->InitializeRenderPassComponent(m_RenderPassComp);

    InitializeResourceBindingLayoutDescs();

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

void DebugPass::InitializeResourceBindingLayoutDescs()
{
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResource = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Vertex;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResource = m_debugMaterialGPUBufferComp;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Vertex;
}

bool DebugPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus DebugPass::GetStatus()
{
	return m_ObjectStatus;
}

bool DebugPass::PrepareCommandList(IRenderingContext* renderingContext)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();

	if (l_renderingConfig.drawDebugObject)
	{
		auto l_sphere = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(ProceduralMeshShape::Sphere);
		auto l_cube = g_Engine->Get<TemplateAssetService>()->GetMeshComponent(ProceduralMeshShape::Cube);

		m_debugSphereConstantBuffer.clear();
		m_debugCubeConstantBuffer.clear();
		m_debugCameraFrustumConstantBuffer.clear();
		m_debugMaterialConstantBuffer.clear();

		for (size_t i = 0; i < 6; i++)
		{
			m_debugMaterialConstantBuffer.emplace_back();
		}

		m_debugMaterialConstantBuffer[0].color = Vec4(0.1f, 0.2f, 0.4f, 1.0f);
		m_debugMaterialConstantBuffer[1].color = Vec4(0.4f, 0.2f, 0.1f, 1.0f);
		m_debugMaterialConstantBuffer[2].color = Vec4(0.9f, 0.1f, 0.0f, 1.0f);
		m_debugMaterialConstantBuffer[3].color = Vec4(0.8f, 0.1f, 0.1f, 1.0f);
		m_debugMaterialConstantBuffer[4].color = Vec4(0.1f, 0.6f, 0.2f, 1.0f);
		m_debugMaterialConstantBuffer[5].color = Vec4(0.1f, 0.3f, 0.5f, 1.0f);

		static bool l_drawProbes = false;
		if (l_drawProbes)
		{
			auto l_probes = GIDataLoader::GetProbes();

			if (l_probes.size() > 0)
			{
				for (size_t i = 0; i < l_probes.size(); i++)
				{
					DebugPerObjectConstantBuffer l_meshData;

					l_meshData.m = Math::toTranslationMatrix(l_probes[i].pos);
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
					auto l_meshData = AddAABB(l_bricks[i].boundBox);
					l_meshData.materialID = 1;

					m_debugCubeConstantBuffer.emplace_back(l_meshData);
				}
			}
		}

		static bool l_drawBVHNodes = false;
		if (l_drawBVHNodes)
		{
			auto l_BVHNodes = g_Engine->Get<PhysicsSystem>()->getBVHNodes();

			for (auto& i : l_BVHNodes)
			{
				AddBVHData(i);
			}
		}

		static bool l_drawCameraFrustums = true;
		if (l_drawCameraFrustums)
		{	
			auto l_visibleSceneAABBWS = g_Engine->Get<PhysicsSystem>()->getVisibleSceneAABB();
			auto l_visibleSceneAABBMeshData = AddAABB(l_visibleSceneAABBWS);
			l_visibleSceneAABBMeshData.materialID = 2;

			m_debugCubeConstantBuffer.emplace_back(l_visibleSceneAABBMeshData);

			auto l_cameraComponent = static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetMainCamera();
			if(l_cameraComponent)
			{			
				auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_cameraComponent->m_Owner);
				auto l_m = l_transformComponent->m_globalTransformMatrix.m_translationMat;
				for (size_t i = 0; i < m_debugCameraFrustumMeshComps.size(); i++)
				{
					std::vector<Vec3> l_vertices;
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
					g_Engine->Get<AssetSystem>()->FulfillVerticesAndIndices(m_debugCameraFrustumMeshComps[i], l_indices, l_vertices, 6);
					l_renderingServer->UpdateMeshComponent(m_debugCameraFrustumMeshComps[i]);

					DebugPerObjectConstantBuffer l_meshData;
					l_meshData.m = Math::generateIdentityMatrix<float>();
					l_meshData.materialID = 3;
					m_debugCameraFrustumConstantBuffer.emplace_back(l_meshData);
				}
				
				auto l_pCamera = l_cameraComponent->m_projectionMatrix;
				auto l_rCamera = Math::toRotationMatrix(l_transformComponent->m_globalTransformVector.m_rot);
				auto l_tCamera = Math::toTranslationMatrix(l_transformComponent->m_globalTransformVector.m_pos);

				auto l_vertices = Math::generateFrustumVerticesWS(l_pCamera, l_rCamera, l_tCamera);

				for (auto& j : l_vertices)
				{
					DebugPerObjectConstantBuffer l_meshData;
					auto l_s = Math::toScaleMatrix(Vec4(0.1f, 0.1f, 0.1f, 0.1f));
					l_meshData.m = Math::toTranslationMatrix(Vec4(j.m_pos, 1.0f));
					l_meshData.m = l_meshData.m * l_s;
					l_meshData.materialID = 4;
					m_debugSphereConstantBuffer.emplace_back(l_meshData);
				}
			}

			auto l_lightComponents = g_Engine->Get<ComponentManager>()->GetAll<LightComponent>();
			for (auto& i : l_lightComponents)
			{
				if(i->m_LightType == LightType::Directional)
				{
					auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(i->m_Owner);
					auto l_r = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
					auto l_rInv = l_r.inverse();

					DebugPerObjectConstantBuffer l_meshData;

					auto l_visibleSceneAABBWS_Extended = Math::extendAABBToBoundingSphere(l_visibleSceneAABBWS);

					auto l_t = Math::toTranslationMatrix(l_visibleSceneAABBWS_Extended.m_center);
					auto l_s = Math::generateIdentityMatrix<float>();
					l_s.m00 *= l_visibleSceneAABBWS_Extended.m_extend.x / 2.0f;
					l_s.m11 *= l_visibleSceneAABBWS_Extended.m_extend.y / 2.0f;
					l_s.m22 *= l_visibleSceneAABBWS_Extended.m_extend.z / 2.0f;

					l_meshData.m = l_t * l_r * l_s;
					l_meshData.materialID = 2;

					m_debugCubeConstantBuffer.emplace_back(l_meshData);

					auto l_visibleSceneAABBLS = Math::rotateAABBToNewSpace(l_visibleSceneAABBWS_Extended, l_rInv);

					for (size_t j = 0; j < i->m_SplitAABBWS.size(); j++)
					{
						{
							DebugPerObjectConstantBuffer l_meshData;

							auto l_aabbLS = i->m_SplitAABBLS[j];
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
							auto l_centerWS = Math::mul(l_aabbLS.m_center, l_r);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
							auto l_centerWS = Math::mul(l_r, l_aabbLS.m_center);
#endif

							auto l_t = Math::toTranslationMatrix(l_centerWS);
							auto l_s = Math::generateIdentityMatrix<float>();
							l_s.m00 *= l_aabbLS.m_extend.x / 2.0f;
							l_s.m11 *= l_aabbLS.m_extend.y / 2.0f;
							l_s.m22 *= l_aabbLS.m_extend.z / 2.0f;

							l_meshData.m = l_t * l_r * l_s;
							l_meshData.materialID = 5;

							m_debugCubeConstantBuffer.emplace_back(l_meshData);
						}
						{
							auto l_meshData = AddAABB(i->m_SplitAABBWS[j]);
							l_meshData.materialID = 4;

							m_debugCubeConstantBuffer.emplace_back(l_meshData);
						}
					}
				}
			}
		}

		static bool l_drawSkeletons = true;
		if (l_drawSkeletons)
		{
			auto l_visibleComponents = g_Engine->Get<ComponentManager>()->GetAll<VisibleComponent>();

			for (auto i : l_visibleComponents)
			{
				if (i->m_meshUsage == MeshUsage::Skeletal && i->m_model)
				{
					auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(i->m_Owner);
					auto l_m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

					for (size_t j = 0; j < i->m_model->meshMaterialPairs.m_count; j++)
					{
						auto l_pair = g_Engine->Get<AssetSystem>()->GetMeshMaterialPair(i->m_model->meshMaterialPairs.m_startOffset + j);
						auto l_skeleton = l_pair->mesh->m_SkeletonComp;

						for (auto k : l_skeleton->m_BoneData)
						{
							DebugPerObjectConstantBuffer l_meshData;
							auto l_bm = k.m_L2B;
							// Inverse-Joint-Matrix
							l_bm = l_bm.inverse();
							auto l_s = Math::toScaleMatrix(Vec4(0.01f, 0.01f, 0.01f, 1.0f));
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

		l_renderingServer->UploadGPUBufferComponent(m_debugMaterialGPUBufferComp, m_debugMaterialConstantBuffer, 0, m_debugMaterialConstantBuffer.size());
		if (m_debugSphereConstantBuffer.size())
		{
			l_renderingServer->UploadGPUBufferComponent(m_debugSphereMeshGPUBufferComp, m_debugSphereConstantBuffer, 0, m_debugSphereConstantBuffer.size());
		}
		if (m_debugCubeConstantBuffer.size())
		{
			l_renderingServer->UploadGPUBufferComponent(m_debugCubeMeshGPUBufferComp, m_debugCubeConstantBuffer, 0, m_debugCubeConstantBuffer.size());
		}
		if (m_debugCameraFrustumConstantBuffer.size())
		{
			l_renderingServer->UploadGPUBufferComponent(m_debugCameraFrustumGPUBufferComp, m_debugCameraFrustumConstantBuffer, 0, m_debugCameraFrustumConstantBuffer.size());
		}

		l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
		l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
		l_renderingServer->ClearRenderTargets(m_RenderPassComp);

		if (m_debugSphereConstantBuffer.size())
		{
			l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_debugSphereMeshGPUBufferComp, 1);
			l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, l_sphere, m_debugSphereConstantBuffer.size());
		}

		if (m_debugCubeConstantBuffer.size())
		{
			l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_debugCubeMeshGPUBufferComp, 1);
			l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, l_cube, m_debugCubeConstantBuffer.size());
		}

		if (m_debugCameraFrustumConstantBuffer.size())
		{
			for (size_t i = 0; i < m_debugCameraFrustumMeshComps.size(); i++)
			{
				l_renderingServer->BindGPUResource(m_RenderPassComp, ShaderStage::Vertex, m_debugCameraFrustumGPUBufferComp, 1, i, 1);
				l_renderingServer->DrawIndexedInstanced(m_RenderPassComp, m_debugCameraFrustumMeshComps[i], 1);
			}
		}

		l_renderingServer->CommandListEnd(m_RenderPassComp);
	}
	else
	{
		l_renderingServer->CommandListBegin(m_RenderPassComp, 0);
		l_renderingServer->BindRenderPassComponent(m_RenderPassComp);
		l_renderingServer->ClearRenderTargets(m_RenderPassComp);

		l_renderingServer->CommandListEnd(m_RenderPassComp);
	}

	return true;
}

RenderPassComponent* DebugPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

DebugPerObjectConstantBuffer DebugPass::AddAABB(const AABB& aabb)
{
	DebugPerObjectConstantBuffer l_result;
		
	l_result.m = Math::toTranslationMatrix(aabb.m_center);
	l_result.m.m00 *= aabb.m_extend.x / 2.0f;
	l_result.m.m11 *= aabb.m_extend.y / 2.0f;
	l_result.m.m22 *= aabb.m_extend.z / 2.0f;

	return l_result;
}

bool DebugPass::AddBVHData(const BVHNode& node)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	static bool drawIntermediateBB = false;
	if(node.PDC == nullptr && !drawIntermediateBB)
		return true;

	auto l_cubeMeshData = AddAABB(node.m_AABB);
	l_cubeMeshData.materialID = node.PDC == nullptr ? 3 : 5;

	m_debugCubeConstantBuffer.emplace_back(l_cubeMeshData);

	return true;
}