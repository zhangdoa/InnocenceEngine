#include "PhysicsSimulationService.h"

#include "../Common/MathHelper.h"
#include "../Common/LogService.h"
#include "../Common/DoubleBuffer.h"
#include "EntityRegistry.h"
#include "SceneService.h"
#include "AssetService.h"
#include "BVHService.h"
#include "../Services/IGraphicsService.h"
#include "../Component/TransformComponent.h"
#include "../Component/RigidBodyComponent.h"
#include "../Component/CollisionShapeComponent.h"

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

		void CreatePhysXActor(EntityID Entity);

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		const uint32_t m_MaxComponentCount = 16384;

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

		EntityID m_RootEntity = INVALID_ENTITY;
		std::vector<CullingResult> m_CullingResults;
		mutable std::shared_mutex m_CullingResultsMutex;

		std::function<void()> f_SceneLoadingStartedCallback;
	};
}

bool PhysicsSimulationServiceImpl::Setup()
{
	m_ObjectStatus = ObjectStatus::Created;

	m_VisibleSceneBoundary.Reset();
	m_TotalSceneBoundary.Reset();
	m_StaticSceneBoundary.Reset();

    CreateRootComponent();
	
#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Setup();
#endif

	f_SceneLoadingStartedCallback = [&]()
		{
			Log(Verbose, "Clearing all physics simulation data...");

			Log(Verbose, "All collision components have been destroyed.");

			Log(Verbose, "All top-level collision primitives have been destroyed.");

			g_Engine->Get<BVHService>()->ClearNodes();

			CreateRootComponent();

			m_TotalSceneBoundary.Reset();
			m_StaticSceneBoundary.Reset();
			m_VisibleSceneBoundary.Reset();

			Log(Success, "All physics simulation data has been cleared.");
		};

	g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&f_SceneLoadingStartedCallback, 1);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

void Inno::PhysicsSimulationServiceImpl::CreateRootComponent()
{
}

bool PhysicsSimulationServiceImpl::Update()
{
    if (g_Engine->Get<SceneService>()->IsLoading())
        return true;

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Update();
#endif

	return true;
}

void PhysicsSimulationServiceImpl::CreatePhysXActor(EntityID Entity)
{
	// TODO Phase2-migrate: PhysX actor binding deferred — physics simulation not yet using EntityRegistry components
#if defined INNO_PLATFORM_WIN
#endif
}

bool PhysicsSimulationService::Setup(IServiceConfig* systemConfig)
{
	m_Impl = new PhysicsSimulationServiceImpl();
	

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
	return m_Impl->m_StaticSceneBoundary.m_AABB;
}

AABB PhysicsSimulationService::GetTotalSceneAABB()
{
	return m_Impl->m_TotalSceneBoundary.m_AABB;
}

bool PhysicsSimulationService::AddForce(EntityID Entity, Vec4 Force)
{
#if defined INNO_PLATFORM_WIN
#endif
	return true;
}

bool PhysicsSimulationService::CreateCollisionComponent(EntityID Entity)
{
	auto* l_Registry = g_Engine->Get<EntityRegistry>();

	if (!l_Registry->Has<TransformComponent>(Entity))
		l_Registry->Emplace<TransformComponent>(Entity);

	if (!l_Registry->Has<RigidBodyComponent>(Entity))
		l_Registry->Emplace<RigidBodyComponent>(Entity);

	if (!l_Registry->Has<CollisionShapeComponent>(Entity))
		l_Registry->Emplace<CollisionShapeComponent>(Entity);

	return true;
}