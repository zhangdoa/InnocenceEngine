#include "PhysicsSystem.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"
#include "../ComponentManager/ILightComponentManager.h"

#include "../Common/InnoMathHelper.h"
#include "../Core/InnoLogger.h"
#include "../Core/InnoMemory.h"
#include "../Template/ObjectPool.h"

#if defined INNO_PLATFORM_WIN
#include "../ThirdParty/PhysXWrapper/PhysXWrapper.h"
#endif

#include "../Interface/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace InnoPhysicsSystemNS
{
	bool setup();
	bool update();

	PhysicsDataComponent* AddPhysicsDataComponent(InnoEntity* parentEntity);

	PhysicsDataComponent* generatePhysicsDataComponent(MeshMaterialPair* meshMaterialPair);
	bool generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m);
	ArrayRangeInfo generatePhysicsProxy(VisibleComponent* VC);

	void updateVisibleSceneBoundary(const AABB& rhs);
	void updateTotalSceneBoundary(const AABB& rhs);
	void updateStaticSceneBoundary(const AABB& rhs);

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	Vec4 m_visibleSceneBoundMax;
	Vec4 m_visibleSceneBoundMin;
	AABB m_visibleSceneAABB;

	Vec4 m_totalSceneBoundMax;
	Vec4 m_totalSceneBoundMin;
	AABB m_totalSceneAABB;

	Vec4 m_staticSceneBoundMax;
	Vec4 m_staticSceneBoundMin;
	PhysicsDataComponent* m_RootPhysicsDataComponent = 0;
	BVHNode* m_RootBVHNode = 0;

	TObjectPool<PhysicsDataComponent>* m_PhysicsDataComponentPool;
	std::shared_mutex m_mutex;

	std::vector<PhysicsDataComponent*> m_Components;
	std::vector<PhysicsDataComponent*> m_IntermediateComponents;
	std::vector<BVHNode> m_BVHNodes;
	std::unordered_map<VisibleComponent*, ArrayRangeInfo> m_ComponentOwnerLUT;

	DoubleBuffer<std::vector<CullingData>, true> m_cullingData;

	size_t m_maxBVHDepth = 16;
	std::atomic<size_t> m_BVHWorkloadCount = 0;

	std::function<void()> f_sceneLoadingStartCallback;
}

using namespace InnoPhysicsSystemNS;

bool InnoPhysicsSystemNS::setup()
{
	m_PhysicsDataComponentPool = TObjectPool<PhysicsDataComponent>::Create(32678);

	m_Components.reserve(16384);
	m_IntermediateComponents.reserve(16384);
	m_BVHNodes.reserve(32678);

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().setup();
#endif

	f_sceneLoadingStartCallback = [&]()
	{
		auto l_intermediatePDCCount = m_IntermediateComponents.size();
		for (size_t i = 0; i < l_intermediatePDCCount; i++)
		{
			m_PhysicsDataComponentPool->Destroy(m_IntermediateComponents[i]);
		}
		auto l_PDCCount = m_Components.size();
		for (size_t i = 0; i < l_PDCCount; i++)
		{
			m_PhysicsDataComponentPool->Destroy(m_Components[i]);
		}
		m_Components.clear();
		m_IntermediateComponents.clear();
		m_BVHNodes.clear();

		if (m_RootPhysicsDataComponent)
		{
			m_PhysicsDataComponentPool->Destroy(m_RootPhysicsDataComponent);
		}
		m_RootPhysicsDataComponent = AddPhysicsDataComponent(nullptr);
		m_RootPhysicsDataComponent->m_IsIntermediate = true;

		m_totalSceneBoundMax = InnoMath::minVec4<float>;
		m_totalSceneBoundMax.w = 1.0f;
		m_totalSceneBoundMin = InnoMath::maxVec4<float>;
		m_totalSceneBoundMin.w = 1.0f;

		m_staticSceneBoundMax = InnoMath::minVec4<float>;
		m_staticSceneBoundMax.w = 1.0f;
		m_staticSceneBoundMin = InnoMath::maxVec4<float>;
		m_staticSceneBoundMin.w = 1.0f;
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoPhysicsSystemNS::update()
{
#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().update();
#endif

	return true;
}

PhysicsDataComponent* InnoPhysicsSystemNS::AddPhysicsDataComponent(InnoEntity* parentEntity)
{
	std::unique_lock<std::shared_mutex> lock{ m_mutex };

	auto l_PDC = m_PhysicsDataComponentPool->Spawn();

	l_PDC->m_Owner = parentEntity;
	l_PDC->m_Serializable = false;
	l_PDC->m_ObjectLifespan = ObjectLifespan::Persistence;

	return l_PDC;
}

PhysicsDataComponent* InnoPhysicsSystemNS::generatePhysicsDataComponent(MeshMaterialPair* meshMaterialPair)
{
	auto l_MDC = meshMaterialPair->mesh;
	auto l_PDC = AddPhysicsDataComponent(l_MDC->m_Owner);

	l_PDC->m_AABBLS = InnoMath::generateAABB(&l_MDC->m_vertices[0], l_MDC->m_vertices.size());
	l_PDC->m_SphereLS = InnoMath::generateBoundSphere(l_PDC->m_AABBLS);

	l_PDC->m_MeshMaterialPair = meshMaterialPair;

	InnoLogger::Log(LogLevel::Verbose, "PhysicsSystem: PhysicsDataComponent has been generated for MeshDataComponent:", l_MDC->m_Owner->m_InstanceName.c_str(), ".");

	m_Components.emplace_back(l_PDC);

	m_BVHWorkloadCount++;

	return l_PDC;
}

bool InnoPhysicsSystemNS::generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m)
{
	PDC->m_AABBWS = InnoMath::transformAABBSpace(PDC->m_AABBLS, m);
	PDC->m_SphereWS = InnoMath::generateBoundSphere(PDC->m_AABBWS);

	return true;
}

ArrayRangeInfo InnoPhysicsSystemNS::generatePhysicsProxy(VisibleComponent* VC)
{
	ArrayRangeInfo l_result;
	l_result.m_startOffset = m_Components.size();
	l_result.m_count = VC->m_model->meshMaterialPairs.m_count;

	auto l_transformComponent = GetComponent(TransformComponent, VC->m_Owner);
	auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

	for (uint64_t j = 0; j < VC->m_model->meshMaterialPairs.m_count; j++)
	{
		auto l_meshMaterialPair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(VC->m_model->meshMaterialPairs.m_startOffset + j);

		auto l_PDC = generatePhysicsDataComponent(l_meshMaterialPair);
		l_PDC->m_TransformComponent = l_transformComponent;
		l_PDC->m_VisibleComponent = VC;
		l_PDC->m_MeshUsage = VC->m_meshUsage;

		generateAABBInWorldSpace(l_PDC, l_globalTm);
		if (l_PDC->m_MeshUsage == MeshUsage::Static)
		{
			updateStaticSceneBoundary(l_PDC->m_AABBWS);
		}
		updateTotalSceneBoundary(l_PDC->m_AABBWS);

#if defined INNO_PLATFORM_WIN
		if (VC->m_simulatePhysics)
		{
			if (l_meshMaterialPair->mesh->m_meshSource == MeshSource::Customized)
			{
				PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), false);
			}
			else
			{
				switch (l_meshMaterialPair->mesh->m_proceduralMeshShape)
				{
				case InnoType::ProceduralMeshShape::Triangle:
					break;
				case InnoType::ProceduralMeshShape::Square:
					break;
				case InnoType::ProceduralMeshShape::Pentagon:
					break;
				case InnoType::ProceduralMeshShape::Hexagon:
					break;
				case InnoType::ProceduralMeshShape::Tetrahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case InnoType::ProceduralMeshShape::Cube:
					PhysXWrapper::get().createPxBox(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic));
					break;
				case InnoType::ProceduralMeshShape::Octahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case InnoType::ProceduralMeshShape::Dodecahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case InnoType::ProceduralMeshShape::Icosahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case InnoType::ProceduralMeshShape::Sphere:
					PhysXWrapper::get().createPxSphere(l_PDC, l_transformComponent->m_localTransformVector_target.m_scale.x, (VC->m_meshUsage == MeshUsage::Dynamic));
					break;
				default:
					InnoLogger::Log(LogLevel::Error, "PhysicsSystem: Invalid ProceduralMeshShape!");
					break;
				}
			}
		}
#endif

		l_PDC->m_ObjectStatus = ObjectStatus::Activated;
	}

	return l_result;
}

void InnoPhysicsSystemNS::updateVisibleSceneBoundary(const AABB& rhs)
{
	m_visibleSceneBoundMax = InnoMath::elementWiseMax(rhs.m_boundMax, m_visibleSceneBoundMax);
	m_visibleSceneBoundMin = InnoMath::elementWiseMin(rhs.m_boundMin, m_visibleSceneBoundMin);
}

void InnoPhysicsSystemNS::updateTotalSceneBoundary(const AABB& rhs)
{
	m_totalSceneBoundMax = InnoMath::elementWiseMax(rhs.m_boundMax, m_totalSceneBoundMax);
	m_totalSceneBoundMin = InnoMath::elementWiseMin(rhs.m_boundMin, m_totalSceneBoundMin);
}

void InnoPhysicsSystemNS::updateStaticSceneBoundary(const AABB& rhs)
{
	m_staticSceneBoundMax = InnoMath::elementWiseMax(rhs.m_boundMax, m_staticSceneBoundMax);
	m_staticSceneBoundMin = InnoMath::elementWiseMin(rhs.m_boundMin, m_staticSceneBoundMin);

	m_RootPhysicsDataComponent->m_AABBWS = InnoMath::generateAABB(m_staticSceneBoundMax, m_staticSceneBoundMin);
	m_RootPhysicsDataComponent->m_SphereWS = InnoMath::generateBoundSphere(m_RootPhysicsDataComponent->m_AABBWS);
}

bool InnoPhysicsSystem::setup()
{
	return InnoPhysicsSystemNS::setup();
}

bool InnoPhysicsSystem::initialize()
{
	if (InnoPhysicsSystemNS::m_ObjectStatus == ObjectStatus::Created)
	{
		InnoPhysicsSystemNS::m_ObjectStatus = ObjectStatus::Activated;
		InnoLogger::Log(LogLevel::Success, "PhysicsSystem has been initialized.");
		return true;
	}
	else
	{
		InnoLogger::Log(LogLevel::Error, "PhysicsSystem: Object is not created!");
		return false;
	}
	return true;
}

bool InnoPhysicsSystem::update()
{
	return InnoPhysicsSystemNS::update();
}

bool InnoPhysicsSystem::terminate()
{
	InnoPhysicsSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "PhysicsSystem has been terminated.");
	return true;
}

ObjectStatus InnoPhysicsSystem::getStatus()
{
	return InnoPhysicsSystemNS::m_ObjectStatus;
}

bool generateBVHLeafNodes(BVHNode* parentNode)
{
	if (parentNode->childrenPDCs.size() == 1)
	{
		return true;
	}
	// Find max axis
	float l_maxAxisLength;
	uint32_t l_maxAxis;
	if (parentNode->intermediatePDC->m_AABBWS.m_extend.x > parentNode->intermediatePDC->m_AABBWS.m_extend.y)
	{
		if (parentNode->intermediatePDC->m_AABBWS.m_extend.x > parentNode->intermediatePDC->m_AABBWS.m_extend.z)
		{
			l_maxAxisLength = parentNode->intermediatePDC->m_AABBWS.m_extend.x;
			l_maxAxis = 0;
		}
		else
		{
			l_maxAxisLength = parentNode->intermediatePDC->m_AABBWS.m_extend.z;
			l_maxAxis = 2;
		}
	}
	else
	{
		if (parentNode->intermediatePDC->m_AABBWS.m_extend.y > parentNode->intermediatePDC->m_AABBWS.m_extend.z)
		{
			l_maxAxisLength = parentNode->intermediatePDC->m_AABBWS.m_extend.y;
			l_maxAxis = 1;
		}
		else
		{
			l_maxAxisLength = parentNode->intermediatePDC->m_AABBWS.m_extend.z;
			l_maxAxis = 2;
		}
	}

	// Sort children nodes
	if (l_maxAxis == 0)
	{
		std::sort(parentNode->childrenPDCs.begin(), parentNode->childrenPDCs.end(), [&](PhysicsDataComponent* A, PhysicsDataComponent* B)
			{
				return A->m_AABBWS.m_boundMin.x < B->m_AABBWS.m_boundMin.x;
			});
	}
	else if (l_maxAxis == 1)
	{
		std::sort(parentNode->childrenPDCs.begin(), parentNode->childrenPDCs.end(), [&](PhysicsDataComponent* A, PhysicsDataComponent* B)
			{
				return A->m_AABBWS.m_boundMin.y < B->m_AABBWS.m_boundMin.y;
			});
	}
	else
	{
		std::sort(parentNode->childrenPDCs.begin(), parentNode->childrenPDCs.end(), [&](PhysicsDataComponent* A, PhysicsDataComponent* B)
			{
				return A->m_AABBWS.m_boundMin.z < B->m_AABBWS.m_boundMin.z;
			});
	}

	// Construct middle split points
	auto l_midMin = parentNode->intermediatePDC->m_AABBWS.m_boundMin;
	auto l_midMax = parentNode->intermediatePDC->m_AABBWS.m_boundMax;

	//And sort children nodes
	if (l_maxAxis == 0)
	{
		l_midMin.x += l_maxAxisLength / 2.0f;
		l_midMax.x -= l_maxAxisLength / 2.0f;
	}
	else if (l_maxAxis == 1)
	{
		l_midMin.y += l_maxAxisLength / 2.0f;
		l_midMax.y -= l_maxAxisLength / 2.0f;
	}
	else
	{
		l_midMin.z += l_maxAxisLength / 2.0f;
		l_midMax.z -= l_maxAxisLength / 2.0f;
	}

	// Split children nodes
	std::vector<PhysicsDataComponent*> l_leftChildrenPDCs;
	std::vector<PhysicsDataComponent*> l_rightChildrenPDCs;

	auto l_totalChildrenPDCCount = parentNode->childrenPDCs.size();

	l_leftChildrenPDCs.reserve(l_totalChildrenPDCCount);
	l_rightChildrenPDCs.reserve(l_totalChildrenPDCCount);

	for (size_t i = 0; i < l_totalChildrenPDCCount; i++)
	{
		if (l_maxAxis == 0)
		{
			if (parentNode->childrenPDCs[i]->m_AABBWS.m_boundMax.x < l_midMin.x)
			{
				l_leftChildrenPDCs.emplace_back(parentNode->childrenPDCs[i]);
			}
			else
			{
				l_rightChildrenPDCs.emplace_back(parentNode->childrenPDCs[i]);
			}
		}
		else if (l_maxAxis == 1)
		{
			if (parentNode->childrenPDCs[i]->m_AABBWS.m_boundMax.y < l_midMin.y)
			{
				l_leftChildrenPDCs.emplace_back(parentNode->childrenPDCs[i]);
			}
			else
			{
				l_rightChildrenPDCs.emplace_back(parentNode->childrenPDCs[i]);
			}
		}
		else
		{
			if (parentNode->childrenPDCs[i]->m_AABBWS.m_boundMax.z < l_midMin.z)
			{
				l_leftChildrenPDCs.emplace_back(parentNode->childrenPDCs[i]);
			}
			else
			{
				l_rightChildrenPDCs.emplace_back(parentNode->childrenPDCs[i]);
			}
		}
	}

	parentNode->childrenPDCs.clear();
	parentNode->childrenPDCs.shrink_to_fit();

	// Add intermediate nodes and store children PDC
	if (l_leftChildrenPDCs.size() > 0)
	{
		m_BVHNodes.emplace_back();
		auto l_leftBVHNode = &m_BVHNodes[m_BVHNodes.size() - 1];

		parentNode->leftChildNode = l_leftBVHNode;
		l_leftBVHNode->parentNode = parentNode;
		l_leftBVHNode->depth = parentNode->depth + 1;
		l_leftBVHNode->childrenPDCs = std::move(l_leftChildrenPDCs);
		l_leftBVHNode->childrenPDCs.shrink_to_fit();

		if (l_leftChildrenPDCs.size() != 1)
		{
			auto l_leftPDC = AddPhysicsDataComponent(parentNode->intermediatePDC->m_Owner);

			l_leftPDC->m_AABBWS = InnoMath::generateAABB(l_midMax, parentNode->intermediatePDC->m_AABBWS.m_boundMin);
			l_leftPDC->m_SphereWS = InnoMath::generateBoundSphere(l_leftPDC->m_AABBWS);
			l_leftPDC->m_IsIntermediate = true;

			m_IntermediateComponents.emplace_back(l_leftPDC);

			l_leftBVHNode->intermediatePDC = l_leftPDC;
		}
	}

	if (l_rightChildrenPDCs.size() > 0)
	{
		m_BVHNodes.emplace_back();
		auto l_rightBVHNode = &m_BVHNodes[m_BVHNodes.size() - 1];

		parentNode->rightChildNode = l_rightBVHNode;
		l_rightBVHNode->parentNode = parentNode;
		l_rightBVHNode->depth = parentNode->depth + 1;
		l_rightBVHNode->childrenPDCs = std::move(l_rightChildrenPDCs);
		l_rightBVHNode->childrenPDCs.shrink_to_fit();

		if (l_rightChildrenPDCs.size() != 1)
		{
			auto l_rightPDC = AddPhysicsDataComponent(parentNode->intermediatePDC->m_Owner);

			l_rightPDC->m_AABBWS = InnoMath::generateAABB(parentNode->intermediatePDC->m_AABBWS.m_boundMax, l_midMin);
			l_rightPDC->m_SphereWS = InnoMath::generateBoundSphere(l_rightPDC->m_AABBWS);
			l_rightPDC->m_IsIntermediate = true;

			m_IntermediateComponents.emplace_back(l_rightPDC);

			l_rightBVHNode->intermediatePDC = l_rightPDC;
		}
	}

	return true;
}

void generateBVH(BVHNode* node)
{
	if (node)
	{
		if (node->intermediatePDC)
		{
			generateBVHLeafNodes(node);
		}

		if (node->depth < m_maxBVHDepth)
		{
			generateBVH(node->leftChildNode);
			generateBVH(node->rightChildNode);
		}
	}
}

void InnoPhysicsSystem::updateBVH()
{
	if (m_BVHWorkloadCount)
	{
		for (size_t i = 0; i < m_IntermediateComponents.size(); i++)
		{
			m_PhysicsDataComponentPool->Destroy(m_IntermediateComponents[i]);
		}

		m_IntermediateComponents.clear();
		m_BVHNodes.clear();

		// Result
		m_BVHNodes.emplace_back();
		m_RootBVHNode = &m_BVHNodes[m_BVHNodes.size() - 1];

		m_RootBVHNode->intermediatePDC = m_RootPhysicsDataComponent;
		m_RootBVHNode->childrenPDCs = m_Components;

		generateBVH(m_RootBVHNode);

		m_BVHWorkloadCount = 0;
	}
}

void PlainCulling(const CameraComponent* camera, std::vector<CullingData>& cullingDatas)
{
	auto l_cameraFrustum = camera->m_frustum;

	for (auto PDC : m_Components)
	{
		if (PDC->m_ObjectStatus == ObjectStatus::Activated)
		{
			auto l_transformComponent = PDC->m_TransformComponent;
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

			if (PDC->m_MeshUsage == MeshUsage::Dynamic)
			{
				PDC->m_AABBWS = InnoMath::transformAABBSpace(PDC->m_AABBLS, l_globalTm);
				PDC->m_SphereWS = generateBoundSphere(PDC->m_AABBWS);
			}
			if (InnoMath::intersectCheck(l_cameraFrustum, PDC->m_SphereWS))
			{
				CullingData l_cullingData;

				l_cullingData.m = l_globalTm;
				l_cullingData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
				l_cullingData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
				l_cullingData.mesh = PDC->m_MeshMaterialPair->mesh;
				l_cullingData.material = PDC->m_MeshMaterialPair->material;
				l_cullingData.meshUsage = PDC->m_VisibleComponent->m_meshUsage;
				l_cullingData.UUID = PDC->m_VisibleComponent->m_UUID;

				l_cullingData.visibilityMask |= VisibilityMask::MainCamera;

				cullingDatas.emplace_back(l_cullingData);

				updateVisibleSceneBoundary(PDC->m_AABBWS);
			}

			updateTotalSceneBoundary(PDC->m_AABBWS);
		}
	}
}

void SunShadowCulling(const LightComponent* sun, std::vector<CullingData>& cullingDatas)
{
	auto l_sunTransformComponent = GetComponent(TransformComponent, sun->m_Owner);

	if (l_sunTransformComponent == nullptr)
	{
		return;
	}

	auto l_sunRotationInv = l_sunTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();
	auto l_sphereRadius = (m_visibleSceneAABB.m_boundMax - m_visibleSceneAABB.m_center).length();

	for (auto PDC : m_Components)
	{
		if (PDC->m_ObjectStatus == ObjectStatus::Activated)
		{
			auto l_spherePosLS = InnoMath::mul(l_sunRotationInv, PDC->m_SphereWS.m_center);
			auto l_distance = Vec2(l_spherePosLS.x, l_spherePosLS.y).length();

			if (l_distance < PDC->m_SphereWS.m_radius + l_sphereRadius)
			{
				CullingData l_cullingData;

				auto l_transformComponent = PDC->m_TransformComponent;

				l_cullingData.m = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
				l_cullingData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
				l_cullingData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
				l_cullingData.mesh = PDC->m_MeshMaterialPair->mesh;
				l_cullingData.material = PDC->m_MeshMaterialPair->material;
				l_cullingData.meshUsage = PDC->m_VisibleComponent->m_meshUsage;
				l_cullingData.UUID = PDC->m_VisibleComponent->m_UUID;
				l_cullingData.visibilityMask |= VisibilityMask::Sun;

				cullingDatas.emplace_back(l_cullingData);
			}
		}
	}
}

CullingData generateCullingData(const Frustum& frustum, PhysicsDataComponent* PDC)
{
	auto l_transformComponent = GetComponent(TransformComponent, PDC->m_VisibleComponent->m_Owner);
	auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

	if (PDC->m_VisibleComponent->m_meshUsage == MeshUsage::Dynamic)
	{
		PDC->m_AABBWS = InnoMath::transformAABBSpace(PDC->m_AABBLS, l_globalTm);
		PDC->m_SphereWS = generateBoundSphere(PDC->m_AABBWS);
	}

	CullingData l_cullingData;

	l_cullingData.m = l_globalTm;
	l_cullingData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
	l_cullingData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
	l_cullingData.mesh = PDC->m_MeshMaterialPair->mesh;
	l_cullingData.material = PDC->m_MeshMaterialPair->material;
	l_cullingData.meshUsage = PDC->m_VisibleComponent->m_meshUsage;
	l_cullingData.UUID = PDC->m_VisibleComponent->m_UUID;

	if (InnoMath::intersectCheck(frustum, PDC->m_SphereWS))
	{
		updateVisibleSceneBoundary(PDC->m_AABBWS);
		l_cullingData.visibilityMask = VisibilityMask::MainCamera;
	}
	else
	{
		//@TODO: Culling from sun
		l_cullingData.visibilityMask = VisibilityMask::Sun;
	}

	return l_cullingData;
}

void BVHCulling(BVHNode* node, const Frustum& frustum, std::vector<CullingData>& cullingDatas)
{
	if (node->intermediatePDC)
	{
		if (InnoMath::intersectCheck(frustum, node->intermediatePDC->m_SphereWS))
		{
			if (node->leftChildNode)
			{
				BVHCulling(node->leftChildNode, frustum, cullingDatas);
			}
			if (node->rightChildNode)
			{
				BVHCulling(node->rightChildNode, frustum, cullingDatas);
			}
		}
	}
	auto l_PDCCount = node->childrenPDCs.size();
	for (size_t i = 0; i < l_PDCCount; i++)
	{
		auto l_PDC = node->childrenPDCs[i];
		auto l_cullingData = generateCullingData(frustum, l_PDC);

		cullingDatas.emplace_back(l_cullingData);
	}
}

void InnoPhysicsSystem::updateCulling()
{
	auto l_mainCamera = GetComponentManager(CameraComponent)->GetAllComponents()[0];

	if (l_mainCamera == nullptr)
	{
		return;
	}

	auto l_sun = GetComponentManager(LightComponent)->GetAllComponents()[0];

	if (l_sun == nullptr)
	{
		return;
	}

	m_visibleSceneBoundMax = InnoMath::minVec4<float>;
	m_visibleSceneBoundMax.w = 1.0f;

	m_visibleSceneBoundMin = InnoMath::maxVec4<float>;
	m_visibleSceneBoundMin.w = 1.0f;

	auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();
	auto& l_cullingDataVector = m_cullingData.GetValue();
	l_cullingDataVector.clear();

	if (l_cullingDataVector.capacity() < l_visibleComponents.size())
	{
		m_cullingData.Reserve(l_visibleComponents.size());
	}

	PlainCulling(l_mainCamera, l_cullingDataVector);
	SunShadowCulling(l_sun, l_cullingDataVector);
	//BVHCulling(m_RootBVHNode, l_cameraFrustum, l_cullingDataVector);

	m_visibleSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_visibleSceneBoundMax, InnoPhysicsSystemNS::m_visibleSceneBoundMin);
	m_totalSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_totalSceneBoundMax, InnoPhysicsSystemNS::m_totalSceneBoundMin);
}

const std::vector<CullingData>& InnoPhysicsSystem::getCullingData()
{
	return InnoPhysicsSystemNS::m_cullingData.GetValue();
}

AABB InnoPhysicsSystem::getVisibleSceneAABB()
{
	return InnoPhysicsSystemNS::m_visibleSceneAABB;
}

AABB InnoPhysicsSystem::getStaticSceneAABB()
{
	return InnoPhysicsSystemNS::m_RootPhysicsDataComponent->m_AABBWS;
}

AABB InnoPhysicsSystem::getTotalSceneAABB()
{
	return InnoPhysicsSystemNS::m_totalSceneAABB;
}

BVHNode* InnoPhysicsSystem::getRootBVHNode()
{
	return InnoPhysicsSystemNS::m_RootBVHNode;
}

bool InnoPhysicsSystem::addForce(VisibleComponent* VC, Vec4 force)
{
#if defined INNO_PLATFORM_WIN
	auto l_result = m_ComponentOwnerLUT.find(VC);
	if (l_result != m_ComponentOwnerLUT.end())
	{
		auto l_rangeInfo = l_result->second;

		for (size_t i = 0; i < l_rangeInfo.m_count; i++)
		{
			auto l_PDC = m_Components[l_rangeInfo.m_startOffset + i];
			PhysXWrapper::get().addForce(l_PDC, force);
		}
	}
#endif
	return true;
}

bool InnoPhysicsSystem::generatePhysicsProxy(VisibleComponent* VC)
{
	auto l_result = InnoPhysicsSystemNS::generatePhysicsProxy(VC);
	m_ComponentOwnerLUT.emplace(VC, l_result);

	return true;
}