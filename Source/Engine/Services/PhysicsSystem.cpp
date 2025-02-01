#include "PhysicsSystem.h"

#include "../Common/MathHelper.h"
#include "../Common/LogService.h"
#include "../Common/DoubleBuffer.h"
#include "EntityManager.h"
#include "ComponentManager.h"
#include "SceneSystem.h"
#include "AssetSystem.h"
#include "BVHService.h"

#if defined INNO_PLATFORM_WIN
#include "../ThirdParty/PhysXWrapper/PhysXWrapper.h"
#endif

#include "../Engine.h"
using namespace Inno;

namespace Inno
{
	struct PhysicsSystemImpl
	{
		bool Setup();
		bool Update();
		void UpdatePhysicsComponent(Inno::PhysicsComponent* PDC);
		void RunCulling(std::function<bool(PhysicsComponent*)>&& visiblityCheckCallback, std::function<void(PhysicsComponent*)>&& onPassedCallback);

		PhysicsComponent* AddPhysicsComponent(Entity* parentEntity);

		void CreatePhysXActor(PhysicsComponent* PDC);
		void GenerateAABBInWorldSpace(PhysicsComponent* PDC, const Mat4& localToWorldTransform);
		PhysicsComponent* GeneratePhysicsComponent(TransformComponent* transformComponent, ModelComponent* modelComponent, RenderableSet* renderableSet);
		ArrayRangeInfo GeneratePhysicsComponents(ModelComponent* modelComponent);

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

		Entity* m_RootPDCEntity = 0;
		PhysicsComponent* m_RootPhysicsComponent = 0;
		std::vector<PhysicsComponent*> m_Components;

		std::unordered_map<ModelComponent*, ArrayRangeInfo> m_ComponentOwnerLUT;

		std::vector<CullingResult> m_CullingResults;
		mutable std::shared_mutex m_CullingResultsMutex;

		std::function<void()> f_SceneLoadingStartedCallback;
	};
}

bool PhysicsSystemImpl::Setup()
{
	m_RootPDCEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Persistence, "RootPDCEntity/");

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Setup();
#endif

	m_RootPhysicsComponent = AddPhysicsComponent(m_RootPDCEntity);

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

			if (m_RootPhysicsComponent)
			{
				g_Engine->Get<ComponentManager>()->Destroy(m_RootPhysicsComponent);
			}
			m_RootPhysicsComponent = AddPhysicsComponent(m_RootPDCEntity);

			m_TotalSceneBoundary.Reset();
			m_StaticSceneBoundary.Reset();
			m_VisibleSceneBoundary.Reset();

			Log(Success, "All physics system data has been cleared.");
		};

	g_Engine->Get<SceneSystem>()->AddSceneLoadingStartedCallback(&f_SceneLoadingStartedCallback, 1);

	m_ObjectStatus = ObjectStatus::Created;
	return true;
}

bool PhysicsSystemImpl::Update()
{
    if (g_Engine->Get<SceneSystem>()->isLoadingScene())
        return true;
			
	for (auto PDC : m_Components)
	{
		UpdatePhysicsComponent(PDC);
	}

#if defined INNO_PLATFORM_WIN
	PhysXWrapper::get().Update();
#endif

	return true;
}

void PhysicsSystemImpl::GenerateAABBInWorldSpace(PhysicsComponent* PDC, const Mat4& localToWorldTransform)
{
	PDC->m_AABBWS = Math::TransformAABB(PDC->m_AABBLS, localToWorldTransform);
	PDC->m_SphereWS = Math::GenerateBoundingSphere(PDC->m_AABBWS);
}

void Inno::PhysicsSystemImpl::UpdatePhysicsComponent(Inno::PhysicsComponent* PDC)
{
	if (PDC->m_ObjectStatus != ObjectStatus::Activated)
		return;

	auto l_transformComponent = PDC->m_TransformComponent;
	if (!l_transformComponent)
	{
		Log(Warning, "TransformComponent is missing for PhysicsComponent: ", PDC->m_Owner->m_InstanceName.c_str(), "!");
		return;
	}

	auto l_localToWorldTransform = l_transformComponent->m_globalTransformMatrix.m_transformationMat;
	GenerateAABBInWorldSpace(PDC, l_localToWorldTransform);
	UpdateTotalSceneBoundary(PDC->m_AABBWS);
}

void PhysicsSystemImpl::RunCulling(std::function<bool(PhysicsComponent*)>&& visiblityCheckCallback, std::function<void(PhysicsComponent*)>&& onPassedCallback)
{
	for (auto PDC : m_Components)
	{
		if (PDC->m_ObjectStatus != ObjectStatus::Activated)
			continue;

		if (visiblityCheckCallback(PDC))
		{
			onPassedCallback(PDC);

			UpdateVisibleSceneBoundary(PDC->m_AABBWS);
		}
	}
}

PhysicsComponent* PhysicsSystemImpl::AddPhysicsComponent(Entity* parentEntity)
{
	auto l_PDC = g_Engine->Get<ComponentManager>()->Spawn<PhysicsComponent>(parentEntity, false, ObjectLifespan::Persistence);

	return l_PDC;
}

PhysicsComponent* PhysicsSystemImpl::GeneratePhysicsComponent(TransformComponent* transformComponent, ModelComponent* modelComponent, RenderableSet* renderableSet)
{
	auto l_MeshComp = renderableSet->mesh;
	auto l_PDC = AddPhysicsComponent(l_MeshComp->m_Owner);

	l_PDC->m_TransformComponent = transformComponent;
	l_PDC->m_ModelComponent = modelComponent;
	l_PDC->m_AABBLS = Math::GenerateAABB(&l_MeshComp->m_Vertices[0], l_MeshComp->m_Vertices.size());
	l_PDC->m_SphereLS = Math::GenerateBoundingSphere(l_PDC->m_AABBLS);
	l_PDC->m_RenderableSet = renderableSet;

	UpdatePhysicsComponent(l_PDC);

	if (l_PDC->m_ModelComponent->m_meshUsage == MeshUsage::Static)
		UpdateStaticSceneBoundary(l_PDC->m_AABBWS);

	Log(Verbose, "PhysicsComponent has been generated for MeshComponent:", l_MeshComp->m_Owner->m_InstanceName.c_str(), ".");

	m_Components.emplace_back(l_PDC);

	g_Engine->Get<BVHService>()->AddNode(l_PDC);

	return l_PDC;
}

ArrayRangeInfo PhysicsSystemImpl::GeneratePhysicsComponents(ModelComponent* modelComponent)
{
	ArrayRangeInfo l_result;
	l_result.m_startOffset = m_Components.size();
	l_result.m_count = modelComponent->m_Model->renderableSets.m_count;

	auto l_transformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(modelComponent->m_Owner);

	for (uint64_t j = 0; j < modelComponent->m_Model->renderableSets.m_count; j++)
	{
		auto l_renderableSet = g_Engine->Get<AssetSystem>()->GetRenderableSet(modelComponent->m_Model->renderableSets.m_startOffset + j);
		auto l_PDC = GeneratePhysicsComponent(l_transformComponent, modelComponent, l_renderableSet);
		CreatePhysXActor(l_PDC);

		l_PDC->m_ObjectStatus = ObjectStatus::Activated;
	}

	m_ComponentOwnerLUT.emplace(modelComponent, l_result);

	return l_result;
}

void PhysicsSystemImpl::CreatePhysXActor(PhysicsComponent* PDC)
{
#if defined INNO_PLATFORM_WIN
	auto l_modelComponent = PDC->m_ModelComponent;
	if (!l_modelComponent->m_simulatePhysics)
		return;

	auto l_renderableSet = PDC->m_RenderableSet;
	auto l_transformComponent = PDC->m_TransformComponent;

	switch (l_renderableSet->mesh->m_MeshShape)
	{
	case Type::MeshShape::Customized:
		PhysXWrapper::get().createPxMesh(PDC, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), false);
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
		PhysXWrapper::get().createPxMesh(PDC, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Cube:
		PhysXWrapper::get().createPxBox(PDC, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic));
		break;
	case Type::MeshShape::Octahedron:
		PhysXWrapper::get().createPxMesh(PDC, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Dodecahedron:
		PhysXWrapper::get().createPxMesh(PDC, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Icosahedron:
		PhysXWrapper::get().createPxMesh(PDC, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic), true);
		break;
	case Type::MeshShape::Sphere:
		PhysXWrapper::get().createPxSphere(PDC, l_transformComponent->m_localTransformVector_target.m_scale.x, (l_modelComponent->m_meshUsage == MeshUsage::Dynamic));
		break;
	default:
		Log(Error, "Invalid MeshShape!");
		break;
	}
#endif
}

void PhysicsSystemImpl::UpdateVisibleSceneBoundary(const AABB& rhs)
{
	m_VisibleSceneBoundary.m_Max = Math::elementWiseMax(rhs.m_boundMax, m_VisibleSceneBoundary.m_Max);
	m_VisibleSceneBoundary.m_Min = Math::elementWiseMin(rhs.m_boundMin, m_VisibleSceneBoundary.m_Min);
}

void PhysicsSystemImpl::UpdateTotalSceneBoundary(const AABB& rhs)
{
	m_TotalSceneBoundary.m_Max = Math::elementWiseMax(rhs.m_boundMax, m_TotalSceneBoundary.m_Max);
	m_TotalSceneBoundary.m_Min = Math::elementWiseMin(rhs.m_boundMin, m_TotalSceneBoundary.m_Min);
}

void PhysicsSystemImpl::UpdateStaticSceneBoundary(const AABB& rhs)
{
	m_StaticSceneBoundary.m_Max = Math::elementWiseMax(rhs.m_boundMax, m_StaticSceneBoundary.m_Max);
	m_StaticSceneBoundary.m_Min = Math::elementWiseMin(rhs.m_boundMin, m_StaticSceneBoundary.m_Min);

	m_RootPhysicsComponent->m_AABBWS = Math::GenerateAABB(m_StaticSceneBoundary.m_Max, m_StaticSceneBoundary.m_Min);
	m_RootPhysicsComponent->m_SphereWS = Math::GenerateBoundingSphere(m_RootPhysicsComponent->m_AABBWS);
}

bool PhysicsSystem::Setup(ISystemConfig* systemConfig)
{
	m_Impl = new PhysicsSystemImpl();

	g_Engine->Get<ComponentManager>()->RegisterType<PhysicsComponent>(m_Impl->m_MaxComponentCount, this);

	return m_Impl->Setup();
}

bool PhysicsSystem::Initialize()
{
	if (m_Impl->m_ObjectStatus == ObjectStatus::Created)
	{
		m_Impl->m_ObjectStatus = ObjectStatus::Activated;
		Log(Success, "PhysicsSystem has been initialized.");
		return true;
	}
	else
	{
		Log(Error, "Object is not created!");
		return false;
	}
	return true;
}

bool PhysicsSystem::Update()
{
	return m_Impl->Update();
}

bool PhysicsSystem::Terminate()
{
	m_Impl->m_ObjectStatus = ObjectStatus::Terminated;
	delete m_Impl;
	Log(Success, "PhysicsSystem has been terminated.");
	return true;
}

ObjectStatus PhysicsSystem::GetStatus()
{
	return m_Impl->m_ObjectStatus;
}

void PhysicsSystem::RunCulling()
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

	auto visibilityCheck = [&](PhysicsComponent* PDC) -> bool {
		return Math::IsOverlap(l_mainCamera->m_frustum, PDC->m_SphereWS);
		};

	auto onPassed = [&](PhysicsComponent* PDC) {
		CullingResult l_cullingResult = {};
		l_cullingResult.m_PhysicsComponent = PDC;
		l_cullingResult.m_VisibilityMask |= VisibilityMask::MainCamera;
		m_Impl->m_CullingResults.emplace_back(l_cullingResult);
		};

	m_Impl->RunCulling(std::move(visibilityCheck), std::move(onPassed));

	// auto sunVisibilityCheck = [&](PhysicsComponent* PDC) -> bool {
	// 	auto l_sunTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(l_sun->m_Owner);
	// 	auto l_sunRotationInv = l_sunTransformComponent->m_globalTransformMatrix.m_rotationMat.inverse();
	// 	auto l_sphereRadius = (m_Impl->m_VisibleSceneBoundary.m_Max - m_Impl->m_VisibleSceneBoundary.m_AABB.m_center).length();
	// 	auto l_spherePosLS = l_sunRotationInv * Vec4(PDC->m_SphereWS.m_center, 1.0f);
	// 	auto l_distance = Vec2(l_spherePosLS.x, l_spherePosLS.y).length();
	// 	return l_distance < PDC->m_SphereWS.m_radius + l_sphereRadius;
	// 	};

	// auto onSunPassed = [&](PhysicsComponent* PDC) {
	// 	CullingResult l_cullingResult;
	// 	l_cullingResult.m_PhysicsComponent = PDC;
	// 	l_cullingResult.m_VisibilityMask |= VisibilityMask::Sun;
	// 	l_cullingResults.emplace_back(l_cullingResult);
	// 	};

	// m_Impl->RunCulling(std::move(sunVisibilityCheck), std::move(onSunPassed));

	auto l_BVHService = g_Engine->Get<BVHService>();

	m_Impl->m_VisibleSceneBoundary.m_AABB = Math::GenerateAABB(m_Impl->m_VisibleSceneBoundary.m_Max, m_Impl->m_VisibleSceneBoundary.m_Min);
	m_Impl->m_TotalSceneBoundary.m_AABB = Math::GenerateAABB(m_Impl->m_TotalSceneBoundary.m_Max, m_Impl->m_TotalSceneBoundary.m_Min);
}

const std::vector<CullingResult>& PhysicsSystem::GetCullingResult()
{
	std::lock_guard<std::shared_mutex> l_lock(m_Impl->m_CullingResultsMutex);
	return m_Impl->m_CullingResults;
}

AABB PhysicsSystem::GetVisibleSceneAABB()
{
	return m_Impl->m_VisibleSceneBoundary.m_AABB;
}

AABB PhysicsSystem::GetStaticSceneAABB()
{
	return m_Impl->m_RootPhysicsComponent->m_AABBWS;
}

AABB PhysicsSystem::GetTotalSceneAABB()
{
	return m_Impl->m_TotalSceneBoundary.m_AABB;
}

bool PhysicsSystem::AddForce(ModelComponent* modelComponent, Vec4 force)
{
#if defined INNO_PLATFORM_WIN
	auto l_result = m_Impl->m_ComponentOwnerLUT.find(modelComponent);
	if (l_result != m_Impl->m_ComponentOwnerLUT.end())
	{
		auto l_rangeInfo = l_result->second;

		for (size_t i = 0; i < l_rangeInfo.m_count; i++)
		{
			auto l_PDC = m_Impl->m_Components[l_rangeInfo.m_startOffset + i];
			PhysXWrapper::get().addForce(l_PDC, force);
		}
	}
#endif
	return true;
}

bool PhysicsSystem::GeneratePhysicsProxy(ModelComponent* modelComponent)
{
	auto l_result = m_Impl->GeneratePhysicsComponents(modelComponent);

	return true;
}