#include "../Common/TestRunner.h"
#include "../../Engine/Engine.h"
#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Component/TransformComponent.h"
#include "../../Engine/Component/VisibilityComponent.h"
#include "../../Engine/Component/RigidBodyComponent.h"

using namespace Inno;

void TestEntityRegistrySpawnDestroy()
{
	TestRunner::StartTest("EntityRegistry Spawn and Destroy");

	auto* l_Registry = g_Engine->Get<EntityRegistry>();
	bool l_TestPassed = l_Registry && l_Registry->GetStatus() == ObjectStatus::Activated;

	if (l_TestPassed)
	{
		EntityID l_Entity = l_Registry->Spawn(ObjectLifespan::Scene, "test_spawn");
		l_TestPassed = l_Entity != INVALID_ENTITY && l_Registry->IsValid(l_Entity);

		if (l_TestPassed)
		{
			l_Registry->Destroy(l_Entity);
			l_TestPassed = !l_Registry->IsValid(l_Entity);
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestEntityRegistryComponentEmplaceGet()
{
	TestRunner::StartTest("EntityRegistry Component Emplace and Get");

	struct PositionData { float X = 0.0f, Y = 0.0f, Z = 0.0f; };

	auto* l_Registry = g_Engine->Get<EntityRegistry>();
	bool l_TestPassed = l_Registry != nullptr;

	if (l_TestPassed)
	{
		EntityID l_Entity = l_Registry->Spawn(ObjectLifespan::Scene, "test_component");
		l_TestPassed = l_Entity != INVALID_ENTITY;

		if (l_TestPassed)
		{
			l_Registry->Emplace<PositionData>(l_Entity, PositionData{1.0f, 2.0f, 3.0f});

			auto* l_Pos = l_Registry->Get<PositionData>(l_Entity);
			l_TestPassed = l_Pos && l_Pos->X == 1.0f && l_Pos->Y == 2.0f && l_Pos->Z == 3.0f;
		}

		l_Registry->CleanUp(ObjectLifespan::Scene);
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestEntityRegistryComponentHasRemove()
{
	TestRunner::StartTest("EntityRegistry Component Has and Remove");

	struct TagData { bool Active = false; };

	auto* l_Registry = g_Engine->Get<EntityRegistry>();
	bool l_TestPassed = l_Registry != nullptr;

	if (l_TestPassed)
	{
		EntityID l_Entity = l_Registry->Spawn(ObjectLifespan::Scene, "test_has_remove");
		l_TestPassed = l_Entity != INVALID_ENTITY;

		if (l_TestPassed)
		{
			l_TestPassed = !l_Registry->Has<TagData>(l_Entity);

			l_Registry->Emplace<TagData>(l_Entity, TagData{true});
			l_TestPassed = l_TestPassed && l_Registry->Has<TagData>(l_Entity);

			l_Registry->Remove<TagData>(l_Entity);
			l_TestPassed = l_TestPassed && !l_Registry->Has<TagData>(l_Entity) && l_Registry->Get<TagData>(l_Entity) == nullptr;
		}

		l_Registry->CleanUp(ObjectLifespan::Scene);
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestEntityRegistryCleanUp()
{
	TestRunner::StartTest("EntityRegistry CleanUp by Lifespan");

	struct MarkerData { int Value = 0; };

	auto* l_Registry = g_Engine->Get<EntityRegistry>();
	bool l_TestPassed = l_Registry != nullptr;

	if (l_TestPassed)
	{
		EntityID l_SceneEntity       = l_Registry->Spawn(ObjectLifespan::Scene, "test_cleanup_scene");
		EntityID l_PersistenceEntity = l_Registry->Spawn(ObjectLifespan::Persistence, "test_cleanup_persist");
		l_TestPassed = l_SceneEntity != INVALID_ENTITY && l_PersistenceEntity != INVALID_ENTITY;

		if (l_TestPassed)
		{
			l_Registry->Emplace<MarkerData>(l_SceneEntity, MarkerData{1});
			l_Registry->Emplace<MarkerData>(l_PersistenceEntity, MarkerData{2});

			l_Registry->CleanUp(ObjectLifespan::Scene);

			l_TestPassed = !l_Registry->IsValid(l_SceneEntity) && l_Registry->IsValid(l_PersistenceEntity);
		}

		l_Registry->CleanUp(ObjectLifespan::Persistence);
	}

	TestRunner::EndTest(l_TestPassed);
}

void TestEntityRegistryFreeListRecycle()
{
	TestRunner::StartTest("EntityRegistry Free List Recycling");

	auto* l_Registry = g_Engine->Get<EntityRegistry>();
	bool l_TestPassed = l_Registry != nullptr;

	if (l_TestPassed)
	{
		EntityID l_First = l_Registry->Spawn(ObjectLifespan::Scene, "recycle_first");
		l_TestPassed = l_First != INVALID_ENTITY;

		if (l_TestPassed)
		{
			l_Registry->Destroy(l_First);
			EntityID l_Recycled = l_Registry->Spawn(ObjectLifespan::Scene, "recycle_second");
			l_TestPassed = l_Recycled == l_First && l_Registry->IsValid(l_Recycled);
			l_Registry->Destroy(l_Recycled);
		}
	}

	TestRunner::EndTest(l_TestPassed);
}

static void TestNewComponentTypes()
{
	auto* l_Registry = g_Engine->Get<EntityRegistry>();

	auto l_Entity = l_Registry->Spawn(ObjectLifespan::Frame, "component_type_test");

	// TransformComponent
	auto& l_Transform = l_Registry->Emplace<TransformComponent>(l_Entity);
	l_Transform.m_LocalPos = Vec3(1.f, 2.f, 3.f);
	auto* l_TPtr = l_Registry->Get<TransformComponent>(l_Entity);
	assert(l_TPtr != nullptr && "TransformComponent should be retrievable");
	assert(l_TPtr->m_LocalPos.x == 1.f && "TransformComponent position x should be 1");
	assert(l_TPtr->m_Dirty == true && "TransformComponent should start dirty");

	// VisibilityComponent
	l_Registry->Emplace<VisibilityComponent>(l_Entity);
	auto* l_Vis = l_Registry->Get<VisibilityComponent>(l_Entity);
	assert(l_Vis != nullptr && "VisibilityComponent should be retrievable");
	assert(l_Vis->m_Visible == true && "VisibilityComponent should default to visible");

	// RigidBodyComponent
	l_Registry->Emplace<RigidBodyComponent>(l_Entity);
	assert(l_Registry->Has<RigidBodyComponent>(l_Entity) && "Entity should have RigidBodyComponent");

	// All three coexist
	assert(l_Registry->Has<TransformComponent>(l_Entity));
	assert(l_Registry->Has<VisibilityComponent>(l_Entity));
	assert(l_Registry->Has<RigidBodyComponent>(l_Entity));

	// CleanUp removes Frame-lifespan entities and their components
	l_Registry->CleanUp(ObjectLifespan::Frame);
	assert(!l_Registry->IsValid(l_Entity) && "Entity should be invalid after CleanUp");

	Log(Success, "EntityRegistry new component types -- PASSED");
}

void RunEntityRegistryUnitTests()
{
	TestRunner::StartTestSuite("EntityRegistry Unit Tests");

	TestEntityRegistrySpawnDestroy();
	TestEntityRegistryComponentEmplaceGet();
	TestEntityRegistryComponentHasRemove();
	TestEntityRegistryCleanUp();
	TestEntityRegistryFreeListRecycle();
	TestNewComponentTypes();

	TestRunner::EndTestSuite();
}
