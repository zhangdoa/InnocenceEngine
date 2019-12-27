#include "PhysicsSystem.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../Common/InnoMathHelper.h"
#include "../Core/InnoLogger.h"
#include "../Core/InnoMemory.h"

#if defined INNO_PLATFORM_WIN
#include "PhysXWrapper.h"
#endif

#include "../ModuleManager/IModuleManager.h"
extern IModuleManager* g_pModuleManager;

namespace InnoPhysicsSystemNS
{
	bool setup();
	bool update();

	PhysicsDataComponent* AddPhysicsDataComponent(InnoEntity* parentEntity);

	PhysicsDataComponent* generatePhysicsDataComponent(const ModelPair& modelPair);
	bool generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m);
	bool generatePhysicsProxy(VisibleComponent * VC);

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

	IObjectPool* m_PhysicsDataComponentPool;

	std::vector<PhysicsDataComponent*> m_Components;
	std::vector<PhysicsDataComponent*> m_IntermediateComponents;
	std::vector<BVHNode> m_BVHNodes;

	DoubleBuffer<std::vector<CullingData>, true> m_cullingData;

	size_t m_maxBVHDepth = 16;
	std::atomic<size_t> m_BVHWorkloadCount = 0;

	std::function<void()> f_sceneLoadingStartCallback;
}

using namespace InnoPhysicsSystemNS;

bool InnoPhysicsSystemNS::setup()
{
	m_PhysicsDataComponentPool = InnoMemory::CreateObjectPool<PhysicsDataComponent>(32678);

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

PhysicsDataComponent * InnoPhysicsSystemNS::AddPhysicsDataComponent(InnoEntity * parentEntity)
{
	auto l_rawPtr = m_PhysicsDataComponentPool->Spawn();
	auto l_PDC = new(l_rawPtr)PhysicsDataComponent();

	l_PDC->m_ParentEntity = parentEntity;
	l_PDC->m_ObjectSource = ObjectSource::Runtime;
	l_PDC->m_ObjectOwnership = ObjectOwnership::Engine;
	l_PDC->m_ComponentType = ComponentType::PhysicsDataComponent;

	return l_PDC;
}

PhysicsDataComponent* InnoPhysicsSystemNS::generatePhysicsDataComponent(const ModelPair& modelPair)
{
	auto l_MDC = modelPair.first;
	auto l_PDC = AddPhysicsDataComponent(l_MDC->m_ParentEntity);

	l_PDC->m_AABBLS = InnoMath::generateAABB(&l_MDC->m_vertices[0], l_MDC->m_vertices.size());
	l_PDC->m_SphereLS = InnoMath::generateBoundSphere(l_PDC->m_AABBLS);

	l_PDC->m_ModelPair = modelPair;

	InnoLogger::Log(LogLevel::Verbose, "PhysicsSystem: PhysicsDataComponent has been generated for MeshDataComponent:", l_MDC->m_ParentEntity->m_EntityName.c_str(), ".");

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

bool InnoPhysicsSystemNS::generatePhysicsProxy(VisibleComponent * VC)
{
	for (auto i : VC->m_PDCs)
	{
		i->m_VisibleComponent = VC;
		if (VC->m_meshUsageType == MeshUsageType::Static)
		{
			updateStaticSceneBoundary(i->m_AABBWS);
		}
		updateTotalSceneBoundary(i->m_AABBWS);
	}

#if defined INNO_PLATFORM_WIN
	if (VC->m_simulatePhysics)
	{
		auto l_transformComponent = GetComponent(TransformComponent, VC->m_ParentEntity);
		switch (VC->m_meshShapeType)
		{
		case MeshShapeType::Cube:
			PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector_target.m_pos, l_transformComponent->m_localTransformVector_target.m_rot, l_transformComponent->m_localTransformVector_target.m_scale, (VC->m_meshUsageType == MeshUsageType::Dynamic));
			break;
		case MeshShapeType::Sphere:
			PhysXWrapper::get().createPxSphere(l_transformComponent, l_transformComponent->m_localTransformVector_target.m_pos, l_transformComponent->m_localTransformVector_target.m_scale.x, (VC->m_meshUsageType == MeshUsageType::Dynamic));
			break;
		case MeshShapeType::Custom:
			for (auto i : VC->m_PDCs)
			{
				PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector_target.m_pos, l_transformComponent->m_localTransformVector_target.m_rot, i->m_AABBWS.m_boundMax - i->m_AABBWS.m_boundMin, (VC->m_meshUsageType == MeshUsageType::Dynamic));
			}
			break;
		default:
			break;
		}
	}
#endif

	return true;
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

void InnoPhysicsSystemNS::updateStaticSceneBoundary(const AABB & rhs)
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

PhysicsDataComponent* InnoPhysicsSystem::generatePhysicsDataComponent(const ModelPair& modelPair)
{
	return InnoPhysicsSystemNS::generatePhysicsDataComponent(modelPair);
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
			auto l_leftPDC = AddPhysicsDataComponent(parentNode->intermediatePDC->m_ParentEntity);

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
			auto l_rightPDC = AddPhysicsDataComponent(parentNode->intermediatePDC->m_ParentEntity);

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

void PlainCulling(const Frustum& frustum, std::vector<CullingData>& cullingDatas)
{
	auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();

	for (auto visibleComponent : l_visibleComponents)
	{
		if (visibleComponent->m_visibilityType != VisibilityType::Invisible && visibleComponent->m_ObjectStatus == ObjectStatus::Activated)
		{
			auto l_transformComponent = GetComponent(TransformComponent, visibleComponent->m_ParentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

			for (auto i : visibleComponent->m_PDCs)
			{
				auto l_PDC = i;

				if (l_PDC)
				{
					CullingData l_cullingData;

					l_cullingData.m = l_globalTm;
					l_cullingData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
					l_cullingData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
					l_cullingData.mesh = l_PDC->m_ModelPair.first;
					l_cullingData.material = l_PDC->m_ModelPair.second;
					l_cullingData.visibilityType = visibleComponent->m_visibilityType;
					l_cullingData.meshUsageType = visibleComponent->m_meshUsageType;
					l_cullingData.UUID = visibleComponent->m_UUID;

					if (visibleComponent->m_meshUsageType == MeshUsageType::Dynamic)
					{
						l_PDC->m_AABBWS = InnoMath::transformAABBSpace(l_PDC->m_AABBLS, l_globalTm);
						l_PDC->m_SphereWS = generateBoundSphere(l_PDC->m_AABBWS);
					}

					if (InnoMath::intersectCheck(frustum, l_PDC->m_SphereWS))
					{
						updateVisibleSceneBoundary(l_PDC->m_AABBWS);
						l_cullingData.cullingDataChannel = CullingDataChannel::MainCamera;
					}
					else
					{
						//@TODO: Culling from sun
						l_cullingData.cullingDataChannel = CullingDataChannel::Shadow;
					}

					cullingDatas.emplace_back(l_cullingData);

					updateTotalSceneBoundary(l_PDC->m_AABBWS);
				}
			}
		}
	}
}

CullingData generateCullingData(const Frustum& frustum, PhysicsDataComponent* PDC)
{
	auto l_transformComponent = GetComponent(TransformComponent, PDC->m_VisibleComponent->m_ParentEntity);
	auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

	if (PDC->m_VisibleComponent->m_meshUsageType == MeshUsageType::Dynamic)
	{
		PDC->m_AABBWS = InnoMath::transformAABBSpace(PDC->m_AABBLS, l_globalTm);
		PDC->m_SphereWS = generateBoundSphere(PDC->m_AABBWS);
	}

	CullingData l_cullingData;

	l_cullingData.m = l_globalTm;
	l_cullingData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
	l_cullingData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
	l_cullingData.mesh = PDC->m_ModelPair.first;
	l_cullingData.material = PDC->m_ModelPair.second;
	l_cullingData.visibilityType = PDC->m_VisibleComponent->m_visibilityType;
	l_cullingData.meshUsageType = PDC->m_VisibleComponent->m_meshUsageType;
	l_cullingData.UUID = PDC->m_VisibleComponent->m_UUID;

	if (InnoMath::intersectCheck(frustum, PDC->m_SphereWS))
	{
		updateVisibleSceneBoundary(PDC->m_AABBWS);
		l_cullingData.cullingDataChannel = CullingDataChannel::MainCamera;
	}
	else
	{
		//@TODO: Culling from sun
		l_cullingData.cullingDataChannel = CullingDataChannel::Shadow;
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
	auto l_mainCamera = GetComponentManager(CameraComponent)->GetMainCamera();

	if (l_mainCamera == nullptr)
	{
		return;
	}

	auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_ParentEntity);

	if (l_mainCameraTransformComponent == nullptr)
	{
		return;
	}

	auto l_cameraFrustum = l_mainCamera->m_frustum;

	m_visibleSceneBoundMax = InnoMath::minVec4<float>;
	m_visibleSceneBoundMax.w = 1.0f;

	m_visibleSceneBoundMin = InnoMath::maxVec4<float>;
	m_visibleSceneBoundMin.w = 1.0f;

	auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();

	std::vector<CullingData> l_cullingDataVector;
	l_cullingDataVector.reserve(l_visibleComponents.size());

	PlainCulling(l_cameraFrustum, l_cullingDataVector);
	//BVHCulling(m_RootBVHNode, l_cameraFrustum, l_cullingDataVector);

	m_visibleSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_visibleSceneBoundMax, InnoPhysicsSystemNS::m_visibleSceneBoundMin);
	m_totalSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_totalSceneBoundMax, InnoPhysicsSystemNS::m_totalSceneBoundMin);

	m_cullingData.SetValue(std::move(l_cullingDataVector));
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

BVHNode * InnoPhysicsSystem::getRootBVHNode()
{
	return InnoPhysicsSystemNS::m_RootBVHNode;
}

bool InnoPhysicsSystem::generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m)
{
	return InnoPhysicsSystemNS::generateAABBInWorldSpace(PDC, m);
}

bool InnoPhysicsSystem::generatePhysicsProxy(VisibleComponent * VC)
{
	return InnoPhysicsSystemNS::generatePhysicsProxy(VC);
}