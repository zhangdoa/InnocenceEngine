#include "PhysicsSystem.h"
#include "../Common/CommonMacro.inl"
#include "../ComponentManager/ITransformComponentManager.h"
#include "../ComponentManager/IVisibleComponentManager.h"
#include "../ComponentManager/ICameraComponentManager.h"

#include "../Common/InnoMathHelper.h"

#if defined INNO_PLATFORM_WIN
#include "PhysXWrapper.h"
#endif

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

namespace InnoPhysicsSystemNS
{
	bool setup();
	bool update();

	bool generatePhysicsDataComponent(MeshDataComponent* MDC);
	bool generatePhysicsDataComponent(VisibleComponent* VC);

	void updateCulling();
	void updateVisibleSceneBoundary(const AABB& rhs);
	void updateTotalSceneBoundary(const AABB& rhs);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;

	vec4 m_visibleSceneBoundMax;
	vec4 m_visibleSceneBoundMin;
	AABB m_visibleSceneAABB;

	vec4 m_totalSceneBoundMax;
	vec4 m_totalSceneBoundMin;
	AABB m_totalSceneAABB;

	void* m_PhysicsDataComponentPool;

	std::atomic<bool> m_isCullingDataPackValid = false;
	ThreadSafeVector<CullingDataPack> m_cullingDataPack;

	std::function<void()> f_sceneLoadingStartCallback;
}

bool InnoPhysicsSystemNS::setup()
{
	m_PhysicsDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(PhysicsDataComponent), 16384);

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().setup();
#endif

	f_sceneLoadingStartCallback = [&]() {
		m_cullingDataPack.clear();
		m_isCullingDataPackValid = false;
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
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_PhysicsDataComponentPool, sizeof(PhysicsDataComponent));
	auto l_PDC = new(l_rawPtr)PhysicsDataComponent();

	l_PDC->m_parentEntity = MDC->m_parentEntity;

	auto l_boundMax = InnoMath::minVec4<float>;
	l_boundMax.w = 1.0f;

	auto l_boundMin = InnoMath::maxVec4<float>;
	l_boundMin.w = 1.0f;

	auto l_AABB = InnoMath::generateAABB(&MDC->m_vertices[0], MDC->m_vertices.size());
	auto l_sphere = InnoMath::generateBoundSphere(l_AABB);

	if (InnoMath::isAGreaterThanBVec3(l_AABB.m_boundMax, l_boundMax))
	{
		l_boundMax = l_AABB.m_boundMax;
	}
	if (InnoMath::isALessThanBVec3(l_AABB.m_boundMin, l_boundMin))
	{
		l_boundMin = l_AABB.m_boundMin;
	}
	l_PDC->m_AABB = l_AABB;
	l_PDC->m_sphere = l_sphere;

	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "PhysicsSystem: PhysicsDataComponent has been generated for MeshDataComponent:" + std::string(MDC->m_parentEntity->m_entityName.c_str()) + ".");

	MDC->m_PDC = l_PDC;

	return true;
}

bool InnoPhysicsSystemNS::generatePhysicsDataComponent(VisibleComponent* VC)
{
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_PhysicsDataComponentPool, sizeof(PhysicsDataComponent));
	auto l_PDC = new(l_rawPtr)PhysicsDataComponent();

	l_PDC->m_parentEntity = VC->m_parentEntity;

	auto l_boundMax = InnoMath::minVec4<float>;
	l_boundMax.w = 1.0f;

	auto l_boundMin = InnoMath::maxVec4<float>;
	l_boundMin.w = 1.0f;

	AABB l_AABB;
	Sphere l_sphere;

	for (auto& l_MDC : VC->m_modelMap)
	{
		auto l_AABB = l_MDC.first->m_PDC->m_AABB;
		auto l_sphere = l_MDC.first->m_PDC->m_sphere;

		if (InnoMath::isAGreaterThanBVec3(l_AABB.m_boundMax, l_boundMax))
		{
			l_boundMax = l_AABB.m_boundMax;
		}
		if (InnoMath::isALessThanBVec3(l_AABB.m_boundMin, l_boundMin))
		{
			l_boundMin = l_AABB.m_boundMin;
		}
	}

	l_PDC->m_AABB = InnoMath::generateAABB(l_boundMax, l_boundMin);
	l_PDC->m_sphere = InnoMath::generateBoundSphere(l_PDC->m_AABB);

#if defined INNO_PLATFORM_WIN
	if (VC->m_simulatePhysics)
	{
		auto l_transformComponent = GetComponent(TransformComponent, VC->m_parentEntity);
		switch (VC->m_meshShapeType)
		{
		case MeshShapeType::Cube:
			PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector.m_pos, l_transformComponent->m_localTransformVector.m_rot, l_transformComponent->m_localTransformVector.m_scale);
			break;
		case MeshShapeType::Sphere:
			PhysXWrapper::get().createPxSphere(l_transformComponent, l_transformComponent->m_localTransformVector.m_pos, l_transformComponent->m_localTransformVector.m_scale.x);
			break;
		case MeshShapeType::Custom:
			PhysXWrapper::get().createPxBox(l_transformComponent, l_transformComponent->m_localTransformVector.m_pos, l_transformComponent->m_localTransformVector.m_rot, l_boundMax - l_boundMin);
			break;
		default:
			break;
		}
	}
#endif

	VC->m_PDC = l_PDC;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "PhysicsSystem: PhysicsDataComponent has been generated for VisibleComponent:" + std::string(VC->m_parentEntity->m_entityName.c_str()) + ".");

	return true;
}

bool InnoPhysicsSystem::initialize()
{
	if (InnoPhysicsSystemNS::m_objectStatus == ObjectStatus::Created)
	{
		InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::Activated;
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem has been initialized.");
		return true;
	}
	else
	{
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "PhysicsSystem: Object is not created!");
		return false;
	}
	return true;
}

template<class T>
bool intersectCheck(const TFrustum<T> & lhs, const TAABB<T> & rhs)
{
	auto l_isCenterInside = isPointInFrustum(rhs.m_center, lhs);
	if (l_isCenterInside)
	{
		return true;
	}

	auto l_isMaxInside = isPointInFrustum(rhs.m_boundMax, lhs);
	if (l_isMaxInside)
	{
		return true;
	}

	auto l_isMinInside = isPointInFrustum(rhs.m_boundMin, lhs);
	if (l_isMinInside)
	{
		return true;
	}

	return false;
}

void InnoPhysicsSystemNS::updateCulling()
{
	m_isCullingDataPackValid = false;

	m_cullingDataPack.clear();

	m_visibleSceneBoundMax = InnoMath::minVec4<float>;
	m_visibleSceneBoundMax.w = 1.0f;

	m_visibleSceneBoundMin = InnoMath::maxVec4<float>;
	m_visibleSceneBoundMin.w = 1.0f;

	m_totalSceneBoundMax = InnoMath::minVec4<float>;
	m_totalSceneBoundMax.w = 1.0f;

	m_totalSceneBoundMin = InnoMath::maxVec4<float>;
	m_totalSceneBoundMin.w = 1.0f;

	std::vector<CullingDataPack> l_cullingDataPacks;

	auto l_cameraComponents = GetComponentManager(CameraComponent)->GetAllComponents();

	if (l_cameraComponents.size() > 0)
	{
		auto l_mainCamera = l_cameraComponents[0];
		auto l_mainCameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_parentEntity);

		auto l_cameraFrustum = l_mainCamera->m_frustum;
		auto l_eyeRay = l_mainCamera->m_rayOfEye;

		auto l_visibleComponents = GetComponentManager(VisibleComponent)->GetAllComponents();
		for (auto visibleComponent : l_visibleComponents)
		{
			if (visibleComponent->m_visiblilityType != VisiblilityType::Invisible && visibleComponent->m_objectStatus == ObjectStatus::Activated)
			{
				auto l_transformComponent = GetComponent(TransformComponent, visibleComponent->m_parentEntity);
				auto l_globalTm = l_transformComponent->m_globalTransformMatrix.m_transformationMat;

				if (visibleComponent->m_PDC)
				{
					for (auto& l_modelPair : visibleComponent->m_modelMap)
					{
						auto l_PDC = l_modelPair.first->m_PDC;
						auto l_OBBws = InnoMath::transformAABBSpace(l_PDC->m_AABB, l_globalTm);

						auto l_boundingSphere = Sphere();
						l_boundingSphere.m_center = l_OBBws.m_center;
						l_boundingSphere.m_radius = l_OBBws.m_extend.length();

						if (InnoMath::intersectCheck(l_cameraFrustum, l_boundingSphere))
						{
							updateVisibleSceneBoundary(l_OBBws);

							thread_local CullingDataPack l_cullingDataPack;

							l_cullingDataPack.m = l_globalTm;
							l_cullingDataPack.m_prev = l_transformComponent->m_globalTransformMatrix_prev.m_transformationMat;
							l_cullingDataPack.normalMat = l_transformComponent->m_globalTransformMatrix.m_rotationMat;
							l_cullingDataPack.mesh = l_modelPair.first;
							l_cullingDataPack.material = l_modelPair.second;
							l_cullingDataPack.visiblilityType = visibleComponent->m_visiblilityType;
							l_cullingDataPack.meshUsageType = visibleComponent->m_meshUsageType;
							l_cullingDataPack.UUID = visibleComponent->m_UUID;

							l_cullingDataPacks.emplace_back(l_cullingDataPack);
						}

						updateTotalSceneBoundary(l_OBBws);
					}
				}
			}
		}
	}

	m_visibleSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_visibleSceneBoundMax, InnoPhysicsSystemNS::m_visibleSceneBoundMin);
	m_totalSceneAABB = InnoMath::generateAABB(InnoPhysicsSystemNS::m_totalSceneBoundMax, InnoPhysicsSystemNS::m_totalSceneBoundMin);

	m_cullingDataPack.setRawData(std::move(l_cullingDataPacks));

	m_isCullingDataPackValid = true;
}

void InnoPhysicsSystemNS::updateVisibleSceneBoundary(const AABB& rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	if (boundMax.x > m_visibleSceneBoundMax.x)
	{
		m_visibleSceneBoundMax.x = boundMax.x;
	}
	if (boundMax.y > m_visibleSceneBoundMax.y)
	{
		m_visibleSceneBoundMax.y = boundMax.y;
	}
	if (boundMax.z > m_visibleSceneBoundMax.z)
	{
		m_visibleSceneBoundMax.z = boundMax.z;
	}
	if (boundMin.x < m_visibleSceneBoundMin.x)
	{
		m_visibleSceneBoundMin.x = boundMin.x;
	}
	if (boundMin.y < m_visibleSceneBoundMin.y)
	{
		m_visibleSceneBoundMin.y = boundMin.y;
	}
	if (boundMin.z < m_visibleSceneBoundMin.z)
	{
		m_visibleSceneBoundMin.z = boundMin.z;
	}
}

void InnoPhysicsSystemNS::updateTotalSceneBoundary(const AABB& rhs)
{
	auto boundMax = rhs.m_boundMax;
	auto boundMin = rhs.m_boundMin;

	if (boundMax.x > m_totalSceneBoundMax.x)
	{
		m_totalSceneBoundMax.x = boundMax.x;
	}
	if (boundMax.y > m_totalSceneBoundMax.y)
	{
		m_totalSceneBoundMax.y = boundMax.y;
	}
	if (boundMax.z > m_totalSceneBoundMax.z)
	{
		m_totalSceneBoundMax.z = boundMax.z;
	}
	if (boundMin.x < m_totalSceneBoundMin.x)
	{
		m_totalSceneBoundMin.x = boundMin.x;
	}
	if (boundMin.y < m_totalSceneBoundMin.y)
	{
		m_totalSceneBoundMin.y = boundMin.y;
	}
	if (boundMin.z < m_totalSceneBoundMin.z)
	{
		m_totalSceneBoundMin.z = boundMin.z;
	}
}

bool InnoPhysicsSystemNS::update()
{
	if (g_pModuleManager->getFileSystem()->isLoadingScene())
	{
		return true;
	}

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().update();
#endif

	g_pModuleManager->getTaskSystem()->submit("CullingTask", [&]()
	{
		updateCulling();
	});

	return true;
}

bool InnoPhysicsSystem::update()
{
	return InnoPhysicsSystemNS::update();
}

bool InnoPhysicsSystem::terminate()
{
	InnoPhysicsSystemNS::m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem has been terminated.");
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

std::optional<std::vector<CullingDataPack>> InnoPhysicsSystem::getCullingDataPack()
{
	if (InnoPhysicsSystemNS::m_isCullingDataPackValid)
	{
		return InnoPhysicsSystemNS::m_cullingDataPack.getRawData();
	}

	return std::nullopt;
}

AABB InnoPhysicsSystem::getVisibleSceneAABB()
{
	return InnoPhysicsSystemNS::m_visibleSceneAABB;
}

AABB InnoPhysicsSystem::getTotalSceneAABB()
{
	return InnoPhysicsSystemNS::m_totalSceneAABB;
}

bool InnoPhysicsSystem::generatePhysicsDataComponent(VisibleComponent * VC)
{
	return InnoPhysicsSystemNS::generatePhysicsDataComponent(VC);
}