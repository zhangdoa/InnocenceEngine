#include "PhysicsSimulationService.h"

#include "../Common/MathHelper.h"
#include "../Common/LogService.h"
#include "../Common/DoubleBuffer.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "SceneService.h"
#include "AssetService.h"
#include "BVHService.h"
#include "../RenderingServer/IRenderingServer.h"

#if defined INNO_PLATFORM_WIN
#include "../ThirdParty/PhysXWrapper/PhysXWrapper.h"
#endif

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct PhysicsSimulationServiceImpl
	{
        bool Setup();
        void CreateRootComponent();
		bool Update();
		void UpdateCollisionComponent(Inno::CollisionComponent* CollisionComponent);
		void RunCulling(std::function<bool(CollisionComponent*)>&& visiblityCheckCallback, std::function<void(CollisionComponent*)>&& onPassedCallback);

		CollisionComponent* AddCollisionComponent(Entity* parentEntity);

		void CreatePhysXActor(CollisionComponent* CollisionComponent);
		void GenerateAABBInWorldSpace(CollisionComponent* CollisionComponent, const Mat4& localToWorldTransform);
		CollisionComponent* CreateCollisionComponent(TransformComponent* transformComponent, ModelComponent* modelComponent, RenderableSet* renderableSet);
		ArrayRangeInfo CreateCollisionComponents(ModelComponent* modelComponent);

		void UpdateVisibleSceneBoundary(const AABB& rhs);
		void UpdateTotalSceneBoundary(const AABB& rhs);
		void UpdateStaticSceneBoundary(const AABB& rhs);

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		const size_t m_MaxComponentCount = 16384;

		struct SceneBoundary
		{
			Vec4 m_Max = Math::minVec4<float>;
			Vec4 m_Min = Math::maxVec4<float>;
			AABB m_AABB = {};

			void Reset()
			{
				m_Max = Math::minVec4<float>;
				m_Max.w = 1.0f;
				m_Min = Math::maxVec4<float>;
				m_Min.w = 1.0f;
			}
		};

		SceneBoundary m_VisibleSceneBoundary;
		SceneBoundary m_TotalSceneBoundary;
		SceneBoundary m_StaticSceneBoundary;

		Entity* m_RootCollisionComponentEntity = 0;
		CollisionComponent* m_RootCollisionComponent = 0;
		std::vector<CollisionComponent*> m_Components;
		TObjectPool<CollisionPrimitive>* m_CollisionPrimitivePool;
		std::unordered_map<MeshComponent*, CollisionPrimitive*> m_BottomLevelPrimitives;
		std::unordered_set<CollisionPrimitive*> m_TopLevelPrimitives;

		std::unordered_map<ModelComponent*, ArrayRangeInfo> m_ComponentsPerModelComponent;

		std::vector<CullingResult> m_CullingResults;
		mutable std::shared_mutex m_CullingResultsMutex;

		std::function<void()> f_SceneLoadingStartedCallback;
	};
}

bool PhysicsSimulationServiceImpl::Setup()
{
	// @TODO: Better not to hardcode the pool size.
	m_CollisionPrimitivePool = TObjectPool<CollisionPrimitive>::Create(m_MaxComponentCount * 8);

    CreateRootComponent();
	
#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Setup();
#endif

	f_SceneLoadingStartedCallback = [&]()
		{
			Log(Verbose, "Clearing all physics system data...");

			auto l_componentsCount = m_Components.size();
			for (size_t i = 0; i < l_componentsCount; i++)
			{
				g_Engine->Get<ComponentManager>()->Destroy(m_Components[i]);
			}
			m_Components.clear();

			g_Engine->Get<BVHService>()->ClearNodes();

			CreateRootComponent();

			m_TotalSceneBoundary.Reset();
			m_StaticSceneBoundary.Reset();
			m_VisibleSceneBoundary.Reset();

			Log(Success, "All physics system data has been cleared.");
		};

	g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&f_SceneLoadingStartedCallback, 1);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

void Inno::PhysicsSimulationServiceImpl::CreateRootComponent()
{
	if (m_RootCollisionComponentEntity)
	{
		g_Engine->Get<EntityManager>()->Destroy(m_RootCollisionComponentEntity);
	}

	if (m_RootCollisionComponent)
	{
		m_CollisionPrimitivePool->Destroy(m_RootCollisionComponent->m_BottomLevelCollisionPrimitive);
		m_CollisionPrimitivePool->Destroy(m_RootCollisionComponent->m_TopLevelCollisionPrimitive);
		g_Engine->Get<ComponentManager>()->Destroy(m_RootCollisionComponent);
	}

    m_RootCollisionComponentEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Persistence, "RootCollisionComponentEntity/");
    m_RootCollisionComponent = AddCollisionComponent(m_RootCollisionComponentEntity);
    m_RootCollisionComponent->m_BottomLevelCollisionPrimitive = m_CollisionPrimitivePool->Spawn();
    m_RootCollisionComponent->m_TopLevelCollisionPrimitive = m_CollisionPrimitivePool->Spawn();
    m_RootCollisionComponent->m_ObjectStatus = ObjectStatus::Activated;
}

bool PhysicsSimulationServiceImpl::Update()
{
    if (g_Engine->Get<SceneService>()->IsLoading())
        return true;
			
	for (auto CollisionComponent : m_Components)
	{
		UpdateCollisionComponent(CollisionComponent);
	}

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Update();
#endif

	return true;
}

void PhysicsSimulationServiceImpl::GenerateAABBInWorldSpace(CollisionComponent* CollisionComponent, const Mat4& localToWorldTransform)
{
	CollisionComponent->m_TopLevelCollisionPrimitive->m_AABB = Math::TransformAABB(CollisionComponent->m_BottomLevelCollisionPrimitive->m_AABB, localToWorldTransform);
	CollisionComponent->m_TopLevelCollisionPrimitive->m_Sphere = Math::GenerateBoundingSphere(CollisionComponent->m_BottomLevelCollisionPrimitive->m_AABB);
}

void Inno::PhysicsSimulationServiceImpl::UpdateCollisionComponent(Inno::CollisionComponent* CollisionComponent)
{
	if (CollisionComponent->m_ObjectStatus != ObjectStatus::Activated)
		return;

	auto l_transformComponent = CollisionComponent->m_TransformComponent;
	if (!l_transformComponent)
	{
		Log(Warning, "TransformComponent is missing for CollisionComponent: ", CollisionComponent->m_Owner->m_InstanceName.c_str(), "!");
		return;
	}

	auto l_localToWorldTransform = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
	GenerateAABBInWorldSpace(CollisionComponent, l_localToWorldTransform);
	UpdateTotalSceneBoundary(CollisionComponent->m_TopLevelCollisionPrimitive->m_AABB);
}

void PhysicsSimulationServiceImpl::RunCulling(std::function<bool(CollisionComponent*)>&& visiblityCheckCallback, std::function<void(CollisionComponent*)>&& onPassedCallback)
{
	for (auto CollisionComponent : m_Components)
	{
		if (CollisionComponent->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		if (visiblityCheckCallback(CollisionComponent))
		{
			onPassedCallback(CollisionComponent);

			UpdateVisibleSceneBoundary(CollisionComponent->m_TopLevelCollisionPrimitive->m_AABB);
		}
	}
}

CollisionComponent* PhysicsSimulationServiceImpl::AddCollisionComponent(Entity* parentEntity)
{
	auto l_collisionComponent = g_Engine->Get<ComponentManager>()->Spawn<CollisionComponent>(parentEntity, false, ObjectLifespan::Persistence);

	return l_collisionComponent;
}

CollisionComponent* PhysicsSimulationServiceImpl::CreateCollisionComponent(TransformComponent* transformComponent, ModelComponent* modelComponent, RenderableSet* renderableSet)
{
	auto l_MeshComp = renderableSet->mesh;
	auto l_collisionComponent = AddCollisionComponent(l_MeshComp->m_Owner);

	auto l_collisionPrimitive = m_BottomLevelPrimitives.find(l_MeshComp);
	if (l_collisionPrimitive == m_BottomLevelPrimitives.end())
	{
		auto l_BLCP = m_CollisionPrimitivePool->Spawn();
		l_BLCP->m_AABB = Math::GenerateAABB(&l_MeshComp->m_Vertices[0], l_MeshComp->m_Vertices.size());
		l_BLCP->m_Sphere = Math::GenerateBoundingSphere(l_BLCP->m_AABB);
		m_BottomLevelPrimitives.emplace(l_MeshComp, l_BLCP);
		l_collisionComponent->m_BottomLevelCollisionPrimitive = l_BLCP;
	}
	else
	{
		l_collisionComponent->m_BottomLevelCollisionPrimitive = l_collisionPrimitive->second;
	}

	l_collisionComponent->m_TopLevelCollisionPrimitive = m_CollisionPrimitivePool->Spawn();
	m_TopLevelPrimitives.emplace(l_collisionComponent->m_TopLevelCollisionPrimitive);

	l_collisionComponent->m_TransformComponent = transformComponent;
	l_collisionComponent->m_ModelComponent = modelComponent;
	l_collisionComponent->m_RenderableSet = renderableSet;

	UpdateCollisionComponent(l_collisionComponent);

	if (l_collisionComponent->m_ModelComponent->m_meshUsage == MeshUsage::Static)
		UpdateStaticSceneBoundary(l_collisionComponent->m_TopLevelCollisionPrimitive->m_AABB);

	Log(Verbose, "CollisionComponent has been generated for MeshComponent:", l_MeshComp->m_Owner->m_InstanceName.c_str(), ".");

	m_Components.emplace_back(l_collisionComponent);

	g_Engine->Get<BVHService>()->AddNode(l_collisionComponent);
	g_Engine->getRenderingServer()->Register(l_collisionComponent);

	return l_collisionComponent;
}

ArrayRangeInfo PhysicsSimulationServiceImpl::CreateCollisionComponents(ModelComponent* modelComponent)
{
	ArrayRangeInfo l_result;
	l_result.m_startOffset = m_Components.size();
	l_result.m_count = modelComponent->m_Model->renderableSets.m_count;

	auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(modelComponent->m_Owner);

	for (uint64_t j = 0; j < modelComponent->m_Model->renderableSets.m_count; j++)
	{
		auto l_renderableSet = g_Engine->Get<AssetService>()->GetRenderableSet(modelComponent->m_Model->renderableSets.m_startOffset + j);
		auto l_collisionComponent = CreateCollisionComponent(l_transformComponent, modelComponent, l_renderableSet);
		CreatePhysXActor(l_collisionComponent);

		l_collisionComponent->m_ObjectStatus = ObjectStatus::Activated;
	}

	m_ComponentsPerModelComponent.emplace(modelComponent, l_result);

	return l_result;
}

void PhysicsSimulationServiceImpl::CreatePhysXActor(CollisionComponent* CollisionComponent)
{
#if defined INNO_PLATFORM_WIN
	auto l_modelComponent = CollisionComponent->m_ModelComponent;
	if (!l_modelComponent->m_simulatePhysics)
		return;

	auto l_renderableSet = CollisionComponent->m_RenderableSet;
	auto l_transformComponent = CollisionComponent->m_TransformComponent;

	switch (l_renderableSet->mesh->m_MeshShape)
	{
	case Type::MeshShape::Customized:
		PhysXWrapper::get().createPxMesh(CollisionComponent, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), false);
		break;
	case Type::MeshShape::Triangle:
		break;
	case Type::MeshShape::Square:
		break;
	case Type::MeshShape::Pentagon:
		break;
	case Type::MeshShape::Hexagon:
		break;
	case Type::MeshShape::Tetrahedron:
		PhysXWrapper::get().createPxMesh(CollisionComponent, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Cube:
		PhysXWrapper::get().createPxBox(CollisionComponent, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic));
		break;
	case Type::MeshShape::Octahedron:
		PhysXWrapper::get().createPxMesh(CollisionComponent, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Dodecahedron:
		PhysXWrapper::get().createPxMesh(CollisionComponent, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Icosahedron:
		PhysXWrapper::get().createPxMesh(CollisionComponent, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Sphere:
		PhysXWrapper::get().createPxSphere(CollisionComponent, l_transformComponent->m_localTransformVector_target.m_scale.x, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic));
		break;
	default:
		Log(Error, "Invalid MeshShape!");
		break;
	}
#endif
}

void PhysicsSimulationServiceImpl::UpdateVisibleSceneBoundary(const AABB& rhs)
{
	m_VisibleSceneBoundary.m_Max = Math::elementWiseMax(rhs.m_boundMax, m_VisibleSceneBoundary.m_Max);
	m_VisibleSceneBoundary.m_Min = Math::elementWiseMin(rhs.m_boundMin, m_VisibleSceneBoundary.m_Min);
}

void PhysicsSimulationServiceImpl::UpdateTotalSceneBoundary(const AABB& rhs)
{
	m_TotalSceneBoundary.m_Max = Math::elementWiseMax(rhs.m_boundMax, m_TotalSceneBoundary.m_Max);
	m_TotalSceneBoundary.m_Min = Math::elementWiseMin(rhs.m_boundMin, m_TotalSceneBoundary.m_Min);
}

void PhysicsSimulationServiceImpl::UpdateStaticSceneBoundary(const AABB& rhs)
{
	m_StaticSceneBoundary.m_Max = Math::elementWiseMax(rhs.m_boundMax, m_StaticSceneBoundary.m_Max);
	m_StaticSceneBoundary.m_Min = Math::elementWiseMin(rhs.m_boundMin, m_StaticSceneBoundary.m_Min);

	m_RootCollisionComponent->m_TopLevelCollisionPrimitive->m_AABB = Math::GenerateAABB(m_StaticSceneBoundary.m_Max, m_StaticSceneBoundary.m_Min);
	m_RootCollisionComponent->m_TopLevelCollisionPrimitive->m_Sphere = Math::GenerateBoundingSphere(m_RootCollisionComponent->m_TopLevelCollisionPrimitive->m_AABB);
}

bool PhysicsSimulationService::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new PhysicsSimulationServiceImpl();

	g_Engine->Get<ComponentManager>()->RegisterType<CollisionComponent>(m_Impl->m_MaxComponentCount, this);

	return m_Impl->Setup();
}

bool PhysicsSimulationService::Initialize()
{
	if (m_Impl->m_ObjectStatus == ObjectStatus::Created)
	{
		m_Impl->m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "PhysicsSimulationService has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "Object is not created!");
		return false;
	}
	return true;
}

bool PhysicsSimulationService::Update()
{
	return m_Impl->Update();
}

bool PhysicsSimulationService::Terminate()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Terminated;
	delete m_Impl;
	Log(Success, "PhysicsSimulationService has been terminated.");
	return true;
}

ObjectStatus PhysicsSimulationService::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

void PhysicsSimulationService::RunCulling()
{
	auto l_mainCamera = static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->GetMainCamera();
	if (l_mainCamera == nullptr)
	{
		Log(Warning, "Can't find the main camera.");
		return;
	}

	auto l_sun = g_Engine->Get<ComponentManager>()->Get<LightComponent>(0);
	if (l_sun == nullptr)
	{
		Log(Warning, "Can't find the main light source.");
		return;
	}

	m_Impl->m_VisibleSceneBoundary.Reset();

	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_CullingResultsMutex);

	m_Impl->m_CullingResults.clear();
	m_Impl->m_CullingResults.reserve(m_Impl->m_Components.size());

	auto visibilityCheck = [&](CollisionComponent* CollisionComponent) -> bool {
		return Math::IsOverlap(l_mainCamera->m_frustum, CollisionComponent->m_TopLevelCollisionPrimitive->m_Sphere);
		};

	auto onPassed = [&](CollisionComponent* CollisionComponent) {
		CullingResult l_cullingResult = {};
		l_cullingResult.m_CollisionComponent = CollisionComponent;
		l_cullingResult.m_VisibilityMask |= VisibilityMask::MainCamera;
		m_Impl->m_CullingResults.emplace_back(l_cullingResult);
		};

	m_Impl->RunCulling(std::move(visibilityCheck), std::move(onPassed));

	auto l_sunTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_sun->m_Owner);
	auto l_sunRotationInv = l_sunTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();
	auto l_sphereRadius = (m_Impl->m_VisibleSceneBoundary.m_Max - m_Impl->m_VisibleSceneBoundary.m_AABB.m_center).length();	

	auto sunVisibilityCheck = [&](CollisionComponent* CollisionComponent) -> bool {
		auto l_spherePosLS = l_sunRotationInv * Vec4(CollisionComponent->m_TopLevelCollisionPrimitive->m_Sphere.m_center, 1.0f);
		auto l_distance = Vec2(l_spherePosLS.x, l_spherePosLS.y).length();
		return l_distance < CollisionComponent->m_TopLevelCollisionPrimitive->m_Sphere.m_radius + l_sphereRadius;
		};

	auto onSunPassed = [&](CollisionComponent* CollisionComponent) {
		CullingResult l_cullingResult;
		l_cullingResult.m_CollisionComponent = CollisionComponent;
		l_cullingResult.m_VisibilityMask |= VisibilityMask::Sun;
		m_Impl->m_CullingResults.emplace_back(l_cullingResult);
		};

	m_Impl->RunCulling(std::move(sunVisibilityCheck), std::move(onSunPassed));

	auto l_BVHService = g_Engine->Get<BVHService>();

	m_Impl->m_VisibleSceneBoundary.m_AABB = Math::GenerateAABB(m_Impl->m_VisibleSceneBoundary.m_Max, m_Impl->m_VisibleSceneBoundary.m_Min);
	m_Impl->m_TotalSceneBoundary.m_AABB = Math::GenerateAABB(m_Impl->m_TotalSceneBoundary.m_Max, m_Impl->m_TotalSceneBoundary.m_Min);
}

const std::vector<CullingResult>& PhysicsSimulationService::GetCullingResult()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_CullingResultsMutex);
	return m_Impl->m_CullingResults;
}

AABB PhysicsSimulationService::GetVisibleSceneAABB()
{
	return m_Impl->m_VisibleSceneBoundary.m_AABB;
}

AABB PhysicsSimulationService::GetStaticSceneAABB()
{
	return m_Impl->m_RootCollisionComponent->m_TopLevelCollisionPrimitive->m_AABB;
}

AABB PhysicsSimulationService::GetTotalSceneAABB()
{
	return m_Impl->m_TotalSceneBoundary.m_AABB;
}

bool PhysicsSimulationService::AddForce(ModelComponent* modelComponent, Vec4 force)
{
#if defined INNO_PLATFORM_WIN
	auto l_result = m_Impl->m_ComponentsPerModelComponent.find(modelComponent);
	if (l_result != m_Impl->m_ComponentsPerModelComponent.end())
	{
		auto l_rangeInfo = l_result->second;

		for (size_t i = 0; i < l_rangeInfo.m_count; i++)
		{
			auto l_collisionComponent = m_Impl->m_Components[l_rangeInfo.m_startOffset + i];
			PhysXWrapper::get().addForce(l_collisionComponent, force);
		}
	}
#endif
	return true;
}

bool PhysicsSimulationService::CreateCollisionComponents(ModelComponent* modelComponent)
{
	auto l_result = m_Impl->CreateCollisionComponents(modelComponent);

	return true;
}