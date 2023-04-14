#include "PhysicsSystem.h"

#include "../Common/InnoMathHelper.h"
#include "../Core/InnoLogger.h"

#if defined INNO_PLATFORM_WIN
#include "../ThirdParty/PhysXWrapper/PhysXWrapper.h"
#endif

#include "../Interface/IEngine.h"
using namespace Inno;
extern IEngine* g_Engine;

namespace InnoPhysicsSystemNS
{
	bool Setup();
	bool Update();

	PhysicsComponent* AddPhysicsComponent(InnoEntity* parentEntity);

	PhysicsComponent* generatePhysicsComponent(MeshMaterialPair* meshMaterialPair);
	bool generateAABBInWorldSpace(PhysicsComponent* PDC, const Mat4& m);
	ArrayRangeInfo generatePhysicsProxy(VisibleComponent* VC);

	void updateVisibleSceneBoundary(const AABB& rhs);
	void updateTotalSceneBoundary(const AABB& rhs);
	void updateStaticSceneBoundary(const AABB& rhs);

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	const size_t m_MaxComponentCount = 16384;
	Vec4 m_visibleSceneBoundMax;
	Vec4 m_visibleSceneBoundMin;
	AABB m_visibleSceneAABB;

	Vec4 m_totalSceneBoundMax;
	Vec4 m_totalSceneBoundMin;
	AABB m_totalSceneAABB;

	Vec4 m_staticSceneBoundMax;
	Vec4 m_staticSceneBoundMin;
	InnoEntity* m_RootPDCEntity = 0;
	PhysicsComponent* m_RootPhysicsComponent = 0;

	std::shared_mutex m_mutex;

	std::vector<PhysicsComponent*> m_Components;
	std::vector<BVHNode> m_BVHNodes;
	std::vector<BVHNode> m_TempBVHNodes;

	std::unordered_map<VisibleComponent*, ArrayRangeInfo> m_ComponentOwnerLUT;

	DoubleBuffer<std::vector<CullingData>, true> m_cullingData;

	size_t m_maxBVHDepth = 256;
	std::atomic<size_t> m_BVHWorkloadCount = 0;

	std::function<void()> f_sceneLoadingStartCallback;
}

using namespace InnoPhysicsSystemNS;

bool InnoPhysicsSystemNS::Setup()
{
	m_RootPDCEntity = g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Persistence, "RootPDCEntity/");
	m_BVHNodes.reserve(m_MaxComponentCount);
	m_TempBVHNodes.reserve(m_MaxComponentCount * 2);

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Setup();
#endif

	f_sceneLoadingStartCallback = [&]()
	{
		auto l_componentsCount = m_Components.size();
		for (size_t i = 0; i < l_componentsCount; i++)
		{
			g_Engine->getComponentManager()->Destroy(m_Components[i]);
		}
		m_Components.clear();
		m_BVHNodes.clear();
		m_TempBVHNodes.clear();

		if (m_RootPhysicsComponent)
		{
			g_Engine->getComponentManager()->Destroy(m_RootPhysicsComponent);
		}
		m_RootPhysicsComponent = AddPhysicsComponent(m_RootPDCEntity);

		m_totalSceneBoundMax = InnoMath::minVec4<float>;
		m_totalSceneBoundMax.w = 1.0f;
		m_totalSceneBoundMin = InnoMath::maxVec4<float>;
		m_totalSceneBoundMin.w = 1.0f;

		m_staticSceneBoundMax = InnoMath::minVec4<float>;
		m_staticSceneBoundMax.w = 1.0f;
		m_staticSceneBoundMin = InnoMath::maxVec4<float>;
		m_staticSceneBoundMin.w = 1.0f;
	};

	g_Engine->getSceneSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool InnoPhysicsSystemNS::Update()
{
#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Update();
#endif

	return true;
}

PhysicsComponent* InnoPhysicsSystemNS::AddPhysicsComponent(InnoEntity* parentEntity)
{
	std::unique_lock<std::shared_mutex> lock{ m_mutex };

	auto l_PDC = g_Engine->getComponentManager()->Spawn<PhysicsComponent>(parentEntity, false, ObjectLifespan::Persistence);

	return l_PDC;
}

PhysicsComponent* InnoPhysicsSystemNS::generatePhysicsComponent(MeshMaterialPair* meshMaterialPair)
{
	auto l_MeshComp = meshMaterialPair->mesh;
	auto l_PDC = AddPhysicsComponent(l_MeshComp->m_Owner);

	l_PDC->m_AABBLS = InnoMath::generateAABB(&l_MeshComp->m_Vertices[0], l_MeshComp->m_Vertices.size());
	l_PDC->m_SphereLS = InnoMath::generateBoundSphere(l_PDC->m_AABBLS);

	l_PDC->m_MeshMaterialPair = meshMaterialPair;

	InnoLogger::Log(LogLevel::Verbose, "PhysicsSystem: PhysicsComponent has been generated for MeshComponent:", l_MeshComp->m_Owner->m_InstanceName.c_str(), ".");

	m_Components.emplace_back(l_PDC);

	BVHNode l_BVHNode;
	l_BVHNode.PDC = l_PDC;
	m_BVHNodes.emplace_back(l_BVHNode);

	m_BVHWorkloadCount++;

	return l_PDC;
}

bool InnoPhysicsSystemNS::generateAABBInWorldSpace(PhysicsComponent* PDC, const Mat4& m)
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

	auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(VC->m_Owner);
	auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

	for (uint64_t j = 0; j < VC->m_model->meshMaterialPairs.m_count; j++)
	{
		auto l_meshMaterialPair = g_Engine->getAssetSystem()->getMeshMaterialPair(VC->m_model->meshMaterialPairs.m_startOffset + j);

		auto l_PDC = generatePhysicsComponent(l_meshMaterialPair);
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
			if (l_meshMaterialPair->mesh->m_MeshSource == MeshSource::Customized)
			{
				PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), false);
			}
			else
			{
				switch (l_meshMaterialPair->mesh->m_ProceduralMeshShape)
				{
				case Type::ProceduralMeshShape::Triangle:
					break;
				case Type::ProceduralMeshShape::Square:
					break;
				case Type::ProceduralMeshShape::Pentagon:
					break;
				case Type::ProceduralMeshShape::Hexagon:
					break;
				case Type::ProceduralMeshShape::Tetrahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case Type::ProceduralMeshShape::Cube:
					PhysXWrapper::get().createPxBox(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic));
					break;
				case Type::ProceduralMeshShape::Octahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case Type::ProceduralMeshShape::Dodecahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case Type::ProceduralMeshShape::Icosahedron:
					PhysXWrapper::get().createPxMesh(l_PDC, (VC->m_meshUsage == MeshUsage::Dynamic), true);
					break;
				case Type::ProceduralMeshShape::Sphere:
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

	m_RootPhysicsComponent->m_AABBWS = InnoMath::generateAABB(m_staticSceneBoundMax, m_staticSceneBoundMin);
	m_RootPhysicsComponent->m_SphereWS = InnoMath::generateBoundSphere(m_RootPhysicsComponent->m_AABBWS);
}

bool InnoPhysicsSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getComponentManager()->RegisterType<PhysicsComponent>(m_MaxComponentCount, this);
	return InnoPhysicsSystemNS::Setup();
}

bool InnoPhysicsSystem::Initialize()
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

bool InnoPhysicsSystem::Update()
{
	return InnoPhysicsSystemNS::Update();
}

bool InnoPhysicsSystem::Terminate()
{
	InnoPhysicsSystemNS::m_ObjectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "PhysicsSystem has been terminated.");
	return true;
}

ObjectStatus InnoPhysicsSystem::GetStatus()
{
	return InnoPhysicsSystemNS::m_ObjectStatus;
}

AABB generateAABB(std::vector<BVHNode>::iterator begin, std::vector<BVHNode>::iterator end)
{
	auto l_BoundMax = InnoMath::minVec4<float>;
	l_BoundMax.w = 1.0f;
	auto l_BoundMin = InnoMath::maxVec4<float>;
	l_BoundMin.w = 1.0f;

	for (auto it = begin; it != end; it++)
	{
		l_BoundMax = InnoMath::elementWiseMax(it->m_AABB.m_boundMax, l_BoundMax);
		l_BoundMin = InnoMath::elementWiseMin(it->m_AABB.m_boundMin, l_BoundMin);
	}

	return generateAABB(l_BoundMax, l_BoundMin);
}

bool generateBVH(std::vector<BVHNode>::iterator node, size_t begin, size_t end, std::vector<BVHNode>& nodes)
{
	if (end - begin < 3)
	{
		return true;
	}

	if (node->depth >= m_maxBVHDepth)
	{
		return true;
	}

	// Find max axis
	uint32_t l_maxAxis;
	if (node->m_AABB.m_extend.x > node->m_AABB.m_extend.y)
	{
		if (node->m_AABB.m_extend.x > node->m_AABB.m_extend.z)
		{
			l_maxAxis = 0;
		}
		else
		{
			l_maxAxis = 2;
		}
	}
	else
	{
		if (node->m_AABB.m_extend.y > node->m_AABB.m_extend.z)
		{
			l_maxAxis = 1;
		}
		else
		{
			l_maxAxis = 2;
		}
	}

	auto l_begin = nodes.begin() + begin;
	auto l_end = nodes.begin() + end;

	// Sort children nodes
	std::sort(l_begin, l_end, [&](BVHNode A, BVHNode B)
		{
			return A.m_AABB.m_boundMin[l_maxAxis] < B.m_AABB.m_boundMin[l_maxAxis];
		});

#define SPATIAL_DIVIDE 0
#if SPATIAL_DIVIDE
	auto l_maxAxisLength = node->m_AABB.m_extend[l_maxAxis];
	auto l_middleMaxAxis = node->m_AABB.m_boundMin[l_maxAxis] + l_maxAxisLength / 2.0f;
	auto l_middle = std::find_if(l_begin, l_end, [&](BVHNode A)
		{
			return A.m_AABB.m_boundMin[l_maxAxis] > l_middleMaxAxis;
		});
#else
	auto l_middle = l_begin + (end - begin) / 2;
#endif

	// Add intermediate nodes
	if (l_middle != l_end && l_middle != l_begin)
	{
		BVHNode l_leftChildNode;
		l_leftChildNode.parentNode = node;
		l_leftChildNode.depth = node->depth + 1;
		l_leftChildNode.m_AABB = generateAABB(l_begin, l_middle);

		BVHNode l_rightChildNode;
		l_rightChildNode.parentNode = node;
		l_rightChildNode.depth = node->depth + 1;
		l_rightChildNode.m_AABB = generateAABB(l_middle, l_end);

		nodes.emplace_back(l_leftChildNode);
		node->leftChildNode = nodes.end() - 1;

		nodes.emplace_back(l_rightChildNode);
		node->rightChildNode = nodes.end() - 1;

		auto l_middleRelativeIndex = std::distance(l_begin, l_middle);
		generateBVH(node->leftChildNode, begin, begin + l_middleRelativeIndex, nodes);
		generateBVH(node->rightChildNode, begin + l_middleRelativeIndex, end, nodes);
	}

	return true;
}

void InnoPhysicsSystem::updateBVH()
{
	if (m_BVHWorkloadCount)
	{
		m_BVHWorkloadCount = 0;
	}

	m_TempBVHNodes.clear();

	for (auto& i : m_BVHNodes)
	{
		i.m_AABB = i.PDC->m_AABBWS;
	}

	// the root node
	BVHNode l_BVHNode;
	l_BVHNode.m_AABB = m_totalSceneAABB;

	m_TempBVHNodes.emplace_back(l_BVHNode);
	m_TempBVHNodes.insert(m_TempBVHNodes.end(), m_BVHNodes.begin(), m_BVHNodes.end());

	generateBVH(m_TempBVHNodes.begin(), 1, m_TempBVHNodes.size(), m_TempBVHNodes);
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
	auto l_sunTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(sun->m_Owner);

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

CullingData generateCullingData(const Frustum& frustum, PhysicsComponent* PDC)
{
	auto l_transformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(PDC->m_VisibleComponent->m_Owner);
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

void BVHCulling(std::vector<BVHNode>::iterator node, const Frustum& frustum, std::vector<CullingData>& cullingDatas)
{
	if (InnoMath::intersectCheck(frustum, node->m_AABB))
	{
		if (node->leftChildNode != m_TempBVHNodes.end())
		{
			BVHCulling(node->leftChildNode, frustum, cullingDatas);
		}
		if (node->rightChildNode != m_TempBVHNodes.end())
		{
			BVHCulling(node->rightChildNode, frustum, cullingDatas);
		}
	}

	if (node->PDC)
	{
		auto l_cullingData = generateCullingData(frustum, node->PDC);

		cullingDatas.emplace_back(l_cullingData);
	}
}

void InnoPhysicsSystem::updateCulling()
{
	auto l_mainCamera = static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->GetMainCamera();

	if (l_mainCamera == nullptr)
	{
		return;
	}

	auto l_sun = g_Engine->getComponentManager()->Get<LightComponent>(0);

	if (l_sun == nullptr)
	{
		return;
	}

	m_visibleSceneBoundMax = InnoMath::minVec4<float>;
	m_visibleSceneBoundMax.w = 1.0f;

	m_visibleSceneBoundMin = InnoMath::maxVec4<float>;
	m_visibleSceneBoundMin.w = 1.0f;

	auto l_visibleComponents = g_Engine->getComponentManager()->GetAll<VisibleComponent>();
	auto& l_cullingDataVector = m_cullingData.GetOldValue();
	l_cullingDataVector.clear();

	if (l_cullingDataVector.capacity() < l_visibleComponents.size())
	{
		m_cullingData.Reserve(l_visibleComponents.size());
	}

	PlainCulling(l_mainCamera, l_cullingDataVector);
	//BVHCulling(m_TempBVHNodes.begin() + 1, l_mainCamera->m_frustum, l_cullingDataVector);
	SunShadowCulling(l_sun, l_cullingDataVector);

	m_cullingData.SetValue(std::move(l_cullingDataVector));

	m_visibleSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_visibleSceneBoundMax, InnoPhysicsSystemNS::m_visibleSceneBoundMin);
	m_totalSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_totalSceneBoundMax, InnoPhysicsSystemNS::m_totalSceneBoundMin);
}

const std::vector<CullingData>& InnoPhysicsSystem::getCullingData()
{
	return InnoPhysicsSystemNS::m_cullingData.GetNewValue();
}

AABB InnoPhysicsSystem::getVisibleSceneAABB()
{
	return InnoPhysicsSystemNS::m_visibleSceneAABB;
}

AABB InnoPhysicsSystem::getStaticSceneAABB()
{
	return InnoPhysicsSystemNS::m_RootPhysicsComponent->m_AABBWS;
}

AABB InnoPhysicsSystem::getTotalSceneAABB()
{
	return InnoPhysicsSystemNS::m_totalSceneAABB;
}

const std::vector<BVHNode>& InnoPhysicsSystem::getBVHNodes()
{
	return InnoPhysicsSystemNS::m_TempBVHNodes;
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