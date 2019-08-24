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

struct BVHResult
{
	BVHResult* preResult;
	std::vector<PhysicsDataComponent*> leftChildrenNodes;
	std::vector<PhysicsDataComponent*> rightChildrenNodes;
	PhysicsDataComponent* intermediateLeftNode = 0;
	PhysicsDataComponent* intermediateRightNode = 0;
};

namespace InnoPhysicsSystemNS
{
	bool setup();
	bool update();

	bool generatePhysicsDataComponent(MeshDataComponent* MDC);
	bool generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m);
	bool generatePhysicsProxy(VisibleComponent * VC);

	void updateVisibleSceneBoundary(const AABB& rhs);
	void updateTotalSceneBoundary(const AABB& rhs);
	void updateStaticSceneBoundary(const AABB& rhs);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	Vec4 m_visibleSceneBoundMax;
	Vec4 m_visibleSceneBoundMin;
	AABB m_visibleSceneAABB;

	Vec4 m_totalSceneBoundMax;
	Vec4 m_totalSceneBoundMin;
	AABB m_totalSceneAABB;

	Vec4 m_staticSceneBoundMax;
	Vec4 m_staticSceneBoundMin;
	PhysicsDataComponent m_RootPhysicsDataComponent;

	IObjectPool* m_PhysicsDataComponentPool;

	std::vector<PhysicsDataComponent*> m_Components;
	std::vector<PhysicsDataComponent*> m_IntermediateComponents;
	std::vector<BVHResult> m_BVHResults;

	DoubleBuffer<std::vector<CullingData>, true> m_cullingData;

	std::atomic<size_t> m_BVHWorkloadCount = 0;

	std::function<void()> f_sceneLoadingStartCallback;
}

using namespace InnoPhysicsSystemNS;

bool InnoPhysicsSystemNS::setup()
{
	m_PhysicsDataComponentPool = InnoMemory::CreateObjectPool(sizeof(PhysicsDataComponent), 32678);

	m_Components.reserve(16384);
	m_IntermediateComponents.reserve(16384);
	m_BVHResults.reserve(32678);

	m_RootPhysicsDataComponent.m_IsIntermediate = true;

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
		m_IntermediateComponents.clear();
		m_BVHResults.clear();
		m_RootPhysicsDataComponent = PhysicsDataComponent();

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

	m_objectStatus = ObjectStatus::Created;
	return true;
}

bool InnoPhysicsSystem::setup()
{
	return InnoPhysicsSystemNS::setup();
}

bool InnoPhysicsSystemNS::generatePhysicsDataComponent(MeshDataComponent* MDC)
{
	auto l_rawPtr = m_PhysicsDataComponentPool->Spawn();
	auto l_PDC = new(l_rawPtr)PhysicsDataComponent();

	l_PDC->m_parentEntity = MDC->m_parentEntity;
	l_PDC->m_objectSource = ObjectSource::Runtime;
	l_PDC->m_objectUsage = ObjectUsage::Engine;
	l_PDC->m_ComponentType = ComponentType::PhysicsDataComponent;

	auto l_AABB = InnoMath::generateAABB(&MDC->m_vertices[0], MDC->m_vertices.size());
	auto l_sphere = InnoMath::generateBoundSphere(l_AABB);

	l_PDC->m_AABBLS = l_AABB;
	l_PDC->m_SphereLS = l_sphere;
	l_PDC->m_ParentNode = &m_RootPhysicsDataComponent;

	InnoLogger::Log(LogLevel::Verbose, "PhysicsSystem: PhysicsDataComponent has been generated for MeshDataComponent:", MDC->m_parentEntity->m_entityName.c_str(), ".");

	MDC->m_PDC = l_PDC;
	m_Components.emplace_back(l_PDC);

	m_BVHWorkloadCount++;

	return true;
}

bool InnoPhysicsSystemNS::generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m)
{
	PDC->m_AABBWS = InnoMath::transformAABBSpace(PDC->m_AABBLS, m);
	PDC->m_SphereWS = InnoMath::generateBoundSphere(PDC->m_AABBWS);

	return true;
}

bool InnoPhysicsSystemNS::generatePhysicsProxy(VisibleComponent * VC)
{
	for (auto& l_modelPair : VC->m_modelMap)
	{
		if (l_modelPair.first->m_PDC)
		{
			l_modelPair.first->m_PDC->m_ModelPair = l_modelPair;
			l_modelPair.first->m_PDC->m_VisibleComponent = VC;
			if (VC->m_meshUsageType == MeshUsageType::Static)
			{
				updateStaticSceneBoundary(l_modelPair.first->m_PDC->m_AABBWS);
			}
			updateTotalSceneBoundary(l_modelPair.first->m_PDC->m_AABBWS);
		}
	}

#if defined INNO_PLATFORM_WIN
	if (VC->m_simulatePhysics)
	{
		auto l_transformComponent = GetComponent(TransformComponent, VC->m_parentEntity);
		switch (VC->m_meshShapeType)
		{
		case MeshShapeType::Cube:
			PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector_target.m_pos, l_transformComponent->m_localTransformVector_target.m_rot, l_transformComponent->m_localTransformVector_target.m_scale, (VC->m_meshUsageType == MeshUsageType::Dynamic));
			break;
		case MeshShapeType::Sphere:
			PhysXWrapper::get().createPxSphere(l_transformComponent, l_transformComponent->m_localTransformVector_target.m_pos, l_transformComponent->m_localTransformVector_target.m_scale.x, (VC->m_meshUsageType == MeshUsageType::Dynamic));
			break;
		case MeshShapeType::Custom:
			for (auto& l_modelPair : VC->m_modelMap)
			{
				PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector_target.m_pos, l_transformComponent->m_localTransformVector_target.m_rot, l_modelPair.first->m_PDC->m_AABBWS.m_boundMax - l_modelPair.first->m_PDC->m_AABBWS.m_boundMin, (VC->m_meshUsageType == MeshUsageType::Dynamic));
			}
			break;
		default:
			break;
		}
	}
#endif

	return true;
}

BVHResult* generateBVHLeafNodes(PhysicsDataComponent* parentNode, std::vector<PhysicsDataComponent*>& childrenNodes)
{
	// Find max axis
	float l_maxAxisLength;
	unsigned int l_maxAxis;
	if (parentNode->m_AABBWS.m_extend.x > parentNode->m_AABBWS.m_extend.y)
	{
		if (parentNode->m_AABBWS.m_extend.x > parentNode->m_AABBWS.m_extend.z)
		{
			l_maxAxisLength = parentNode->m_AABBWS.m_extend.x;
			l_maxAxis = 0;
		}
		else
		{
			l_maxAxisLength = parentNode->m_AABBWS.m_extend.z;
			l_maxAxis = 2;
		}
	}
	else
	{
		if (parentNode->m_AABBWS.m_extend.y > parentNode->m_AABBWS.m_extend.z)
		{
			l_maxAxisLength = parentNode->m_AABBWS.m_extend.y;
			l_maxAxis = 1;
		}
		else
		{
			l_maxAxisLength = parentNode->m_AABBWS.m_extend.z;
			l_maxAxis = 2;
		}
	}

	// Construct middle split points
	auto l_midMin = parentNode->m_AABBWS.m_boundMin;
	auto l_midMax = parentNode->m_AABBWS.m_boundMax;

	//And sort children nodes
	if (l_maxAxis == 0)
	{
		l_midMin.x += l_maxAxisLength / 2.0f;
		l_midMax.x -= l_maxAxisLength / 2.0f;

		std::sort(childrenNodes.begin(), childrenNodes.end(), [&](PhysicsDataComponent* A, PhysicsDataComponent* B)
		{
			return A->m_AABBWS.m_boundMin.x < B->m_AABBWS.m_boundMin.x;
		});
	}
	else if (l_maxAxis == 1)
	{
		l_midMin.y += l_maxAxisLength / 2.0f;
		l_midMax.y -= l_maxAxisLength / 2.0f;

		std::sort(childrenNodes.begin(), childrenNodes.end(), [&](PhysicsDataComponent* A, PhysicsDataComponent* B)
		{
			return A->m_AABBWS.m_boundMin.y < B->m_AABBWS.m_boundMin.y;
		});
	}
	else
	{
		l_midMin.z += l_maxAxisLength / 2.0f;
		l_midMax.z -= l_maxAxisLength / 2.0f;

		std::sort(childrenNodes.begin(), childrenNodes.end(), [&](PhysicsDataComponent* A, PhysicsDataComponent* B)
		{
			return A->m_AABBWS.m_boundMin.z < B->m_AABBWS.m_boundMin.z;
		});
	}

	// Split children nodes
	std::vector<PhysicsDataComponent*> l_leftChildrenNodes;
	std::vector<PhysicsDataComponent*> l_rightChildrenNodes;

	auto l_totalChildrenNodesCount = childrenNodes.size();

	l_leftChildrenNodes.reserve(l_totalChildrenNodesCount);
	l_rightChildrenNodes.reserve(l_totalChildrenNodesCount);

	for (size_t i = 0; i < l_totalChildrenNodesCount; i++)
	{
		if (l_maxAxis == 0)
		{
			if (childrenNodes[i]->m_AABBWS.m_boundMin.x < l_midMin.x)
			{
				l_leftChildrenNodes.emplace_back(childrenNodes[i]);
			}
			else
			{
				l_rightChildrenNodes.emplace_back(childrenNodes[i]);
			}
		}
		else if (l_maxAxis == 1)
		{
			if (childrenNodes[i]->m_AABBWS.m_boundMin.y < l_midMin.y)
			{
				l_leftChildrenNodes.emplace_back(childrenNodes[i]);
			}
			else
			{
				l_rightChildrenNodes.emplace_back(childrenNodes[i]);
			}
		}
		else
		{
			if (childrenNodes[i]->m_AABBWS.m_boundMin.z < l_midMin.z)
			{
				l_leftChildrenNodes.emplace_back(childrenNodes[i]);
			}
			else
			{
				l_rightChildrenNodes.emplace_back(childrenNodes[i]);
			}
		}
	}

	childrenNodes.clear();
	childrenNodes.shrink_to_fit();

	// Result
	BVHResult l_result;

	// Add intermediate nodes
	if (l_leftChildrenNodes.size() > 0)
	{
		if (l_leftChildrenNodes.size() == 1)
		{
			parentNode->m_LeftChildNode = l_leftChildrenNodes[0];
			parentNode->m_LeftChildNode->m_ParentNode = parentNode;
		}
		else
		{
			auto l_leftNodeRawPtr = m_PhysicsDataComponentPool->Spawn();
			auto l_leftNode = new(l_leftNodeRawPtr)PhysicsDataComponent();

			l_leftNode->m_AABBWS = InnoMath::generateAABB(l_midMax, parentNode->m_AABBWS.m_boundMin);
			l_leftNode->m_SphereWS = InnoMath::generateBoundSphere(l_leftNode->m_AABBWS);

			l_leftNode->m_IsIntermediate = true;

			m_IntermediateComponents.emplace_back(l_leftNode);

			l_result.intermediateLeftNode = l_leftNode;
			parentNode->m_LeftChildNode = l_leftNode;

			for (auto i : l_leftChildrenNodes)
			{
				i->m_ParentNode = l_leftNode;
			}
		}

		l_result.leftChildrenNodes = std::move(l_leftChildrenNodes);
		l_result.leftChildrenNodes.shrink_to_fit();
	}

	if (l_rightChildrenNodes.size() > 0)
	{
		if (l_rightChildrenNodes.size() == 1)
		{
			parentNode->m_RightChildNode = l_rightChildrenNodes[0];
			parentNode->m_RightChildNode->m_ParentNode = parentNode;
		}
		else
		{
			auto l_rightNodeRawPtr = m_PhysicsDataComponentPool->Spawn();
			auto l_rightNode = new(l_rightNodeRawPtr)PhysicsDataComponent();

			l_rightNode->m_AABBWS = InnoMath::generateAABB(parentNode->m_AABBWS.m_boundMax, l_midMin);
			l_rightNode->m_SphereWS = InnoMath::generateBoundSphere(l_rightNode->m_AABBWS);

			l_rightNode->m_IsIntermediate = true;

			m_IntermediateComponents.emplace_back(l_rightNode);

			l_result.intermediateRightNode = l_rightNode;
			parentNode->m_RightChildNode = l_rightNode;

			for (auto i : l_rightChildrenNodes)
			{
				i->m_ParentNode = l_rightNode;
			}
		}

		l_result.rightChildrenNodes = std::move(l_rightChildrenNodes);
		l_result.rightChildrenNodes.shrink_to_fit();
	}

	m_BVHResults.emplace_back(std::move(l_result));

	return &m_BVHResults[m_BVHResults.size() - 1];
}

bool InnoPhysicsSystem::initialize()
{
	if (InnoPhysicsSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::Activated;
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

	m_RootPhysicsDataComponent.m_AABBWS = InnoMath::generateAABB(m_staticSceneBoundMax, m_staticSceneBoundMin);
	m_RootPhysicsDataComponent.m_SphereWS = InnoMath::generateBoundSphere(m_RootPhysicsDataComponent.m_AABBWS);
}

bool InnoPhysicsSystemNS::update()
{
#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().update();
#endif

	return true;
}

bool InnoPhysicsSystem::update()
{
	return InnoPhysicsSystemNS::update();
}

bool InnoPhysicsSystem::terminate()
{
	InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::Terminated;
	InnoLogger::Log(LogLevel::Success, "PhysicsSystem has been terminated.");
	return true;
}

ObjectStatus InnoPhysicsSystem::getStatus()
{
	return InnoPhysicsSystemNS::m_objectStatus;
}

bool InnoPhysicsSystem::generatePhysicsDataComponent(MeshDataComponent* MDC)
{
	return InnoPhysicsSystemNS::generatePhysicsDataComponent(MDC);
}

bool BVHCulling(PhysicsDataComponent* PDC, const Frustum& frustum, std::vector<CullingData>& cullingDatas)
{
	if (InnoMath::intersectCheck(frustum, PDC->m_SphereWS))
	{
		if (PDC->m_LeftChildNode)
		{
			BVHCulling(PDC->m_LeftChildNode, frustum, cullingDatas);
		}
		if (PDC->m_RightChildNode)
		{
			BVHCulling(PDC->m_RightChildNode, frustum, cullingDatas);
		}

		if (!PDC->m_IsIntermediate)
		{
			updateVisibleSceneBoundary(PDC->m_AABBWS);

			auto l_transformComponent = GetComponent(TransformComponent, PDC->m_VisibleComponent->m_parentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

			CullingData l_cullingData;

			l_cullingData.m = l_globalTm;
			l_cullingData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
			l_cullingData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
			l_cullingData.mesh = PDC->m_ModelPair.first;
			l_cullingData.material = PDC->m_ModelPair.second;
			l_cullingData.visiblilityType = PDC->m_VisibleComponent->m_visiblilityType;
			l_cullingData.meshUsageType = PDC->m_VisibleComponent->m_meshUsageType;
			l_cullingData.UUID = PDC->m_VisibleComponent->m_UUID;

			cullingDatas.emplace_back(l_cullingData);
		}

		return true;
	}

	return false;
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
		m_BVHResults.clear();

		auto l_currentNode = &m_RootPhysicsDataComponent;
		auto l_currentResult = generateBVHLeafNodes(l_currentNode, m_Components);

		l_currentNode = l_currentResult->intermediateLeftNode;

		while (l_currentNode != nullptr)
		{
			if (l_currentResult->leftChildrenNodes.size() > 1)
			{
				auto l_preResult = l_currentResult;

				l_currentResult = generateBVHLeafNodes(l_currentNode, l_currentResult->leftChildrenNodes);
				l_currentResult->preResult = l_preResult;

				l_currentNode = l_currentResult->intermediateLeftNode;

				if (l_currentNode == nullptr)
				{
					l_currentNode = l_currentResult->intermediateRightNode;

					while (l_currentNode != nullptr)
					{
						if (l_currentResult->rightChildrenNodes.size() > 1)
						{
							auto l_preResult = l_currentResult;

							l_currentResult = generateBVHLeafNodes(l_currentNode, l_currentResult->rightChildrenNodes);
							l_currentResult->preResult = l_preResult;

							l_currentNode = l_currentResult->intermediateRightNode;
						}
						else
						{
							l_currentResult = l_currentResult->preResult;
						}
					}
				}
			}
			else
			{
				l_currentResult = l_currentResult->preResult;
			}
		}

		m_BVHWorkloadCount = 0;
	}
}

void InnoPhysicsSystem::updateCulling()
{
	auto l_mainCamera = GetComponentManager(CameraComponent)->GetMainCamera();

	if (l_mainCamera == nullptr)
	{
		return;
	}

	auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);

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

	for (auto visibleComponent : l_visibleComponents)
	{
		if (visibleComponent->m_visiblilityType != VisiblilityType::Invisible && visibleComponent->m_objectStatus == ObjectStatus::Activated)
		{
			auto l_transformComponent = GetComponent(TransformComponent, visibleComponent->m_parentEntity);
			auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

			for (auto& l_modelPair : visibleComponent->m_modelMap)
			{
				auto l_PDC = l_modelPair.first->m_PDC;

				if (l_PDC)
				{
					CullingData l_cullingData;

					l_cullingData.m = l_globalTm;
					l_cullingData.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
					l_cullingData.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
					l_cullingData.mesh = l_modelPair.first;
					l_cullingData.material = l_modelPair.second;
					l_cullingData.visiblilityType = visibleComponent->m_visiblilityType;
					l_cullingData.meshUsageType = visibleComponent->m_meshUsageType;
					l_cullingData.UUID = visibleComponent->m_UUID;

					auto l_OBBws = InnoMath::transformAABBSpace(l_PDC->m_AABBLS, l_globalTm);

					auto l_boundingSphere = Sphere();
					l_boundingSphere.m_center = l_OBBws.m_center;
					l_boundingSphere.m_radius = l_OBBws.m_extend.length();

					if (InnoMath::intersectCheck(l_cameraFrustum, l_boundingSphere))
					{
						updateVisibleSceneBoundary(l_OBBws);
						l_cullingData.cullingDataChannel = CullingDataChannel::MainCamera;
					}
					else
					{
						//@TODO: Culling from sun
						l_cullingData.cullingDataChannel = CullingDataChannel::Shadow;
					}

					l_cullingDataVector.emplace_back(l_cullingData);

					updateTotalSceneBoundary(l_OBBws);
				}
			}
		}
	}

	//BVHCulling(&m_RootPhysicsDataComponent, l_cameraFrustum, l_cullingDatas);

	auto l_t = &m_BVHResults;

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
	return InnoPhysicsSystemNS::m_RootPhysicsDataComponent.m_AABBWS;
}

AABB InnoPhysicsSystem::getTotalSceneAABB()
{
	return InnoPhysicsSystemNS::m_totalSceneAABB;
}

PhysicsDataComponent * InnoPhysicsSystem::getRootPhysicsDataComponent()
{
	return &InnoPhysicsSystemNS::m_RootPhysicsDataComponent;
}

bool InnoPhysicsSystem::generateAABBInWorldSpace(PhysicsDataComponent* PDC, const Mat4& m)
{
	return InnoPhysicsSystemNS::generateAABBInWorldSpace(PDC, m);
}

bool InnoPhysicsSystem::generatePhysicsProxy(VisibleComponent * VC)
{
	return InnoPhysicsSystemNS::generatePhysicsProxy(VC);
}