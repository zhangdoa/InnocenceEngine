#include "TransformSystem.h"
#include "../Component/TransformComponent.h"
#include "../Common/Randomizer.h"
#include "../Common/LogService.h"
#include "ComponentManager.h"
#include "SceneService.h"
#include "EntityManager.h"

#include "../Engine.h"

using namespace Inno;
;

namespace TransformSystemNS
{
	const size_t m_MaxComponentCount = 32768;
	Entity* m_RootTransformEntity;
	TransformComponent* m_RootTransformComponent;

	std::function<void()> f_SceneLoadingFinishCallback;

	void SortTransformComponentsVector()
	{
		auto l_component = g_Engine->Get<ComponentManager>()->GetAll<TransformComponent>();

		//construct the hierarchy tree
		for (auto i : l_component)
		{
			if (i->m_parentTransformComponent)
			{
				i->m_transformHierarchyLevel = i->m_parentTransformComponent->m_transformHierarchyLevel + 1;
			}
		}

		//from top to bottom
		std::sort(l_component.begin(), l_component.end(), [&](TransformComponent* a, TransformComponent* b)
			{
				return a->m_transformHierarchyLevel < b->m_transformHierarchyLevel;
			});
	}

	void SimulateTransformComponents()
	{
		auto l_tickTime = g_Engine->getTickTime();
		auto l_ratio = (1.0f - l_tickTime / 100.0f);
		l_ratio = Math::clamp(l_ratio, 0.01f, 0.99f);

		auto l_component = g_Engine->Get<ComponentManager>()->GetAll<TransformComponent>();

		std::for_each(l_component.begin(), l_component.end(), [&](TransformComponent* val)
			{
				if (!Math::isCloseEnough<float, 4>(val->m_localTransformVector.m_pos, val->m_localTransformVector_target.m_pos))
				{
					val->m_localTransformVector.m_pos = Math::lerp(val->m_localTransformVector.m_pos, val->m_localTransformVector_target.m_pos, l_ratio);
				}

				if (!Math::isCloseEnough<float, 4>(val->m_localTransformVector.m_rot, val->m_localTransformVector_target.m_rot))
				{
					val->m_localTransformVector.m_rot = Math::slerp(val->m_localTransformVector.m_rot, val->m_localTransformVector_target.m_rot, l_ratio);
				}

				if (!Math::isCloseEnough<float, 4>(val->m_localTransformVector.m_scale, val->m_localTransformVector_target.m_scale))
				{
					val->m_localTransformVector.m_scale = Math::lerp(val->m_localTransformVector.m_scale, val->m_localTransformVector_target.m_scale, l_ratio);
				}

				if (val->m_parentTransformComponent)
				{
					val->m_globalTransformVector = Math::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
					val->m_globalTransformMatrix = Math::TransformVectorToTransformMatrix(val->m_globalTransformVector);
				}
			});
	}
}

using namespace TransformSystemNS;

bool TransformSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->Get<ComponentManager>()->RegisterType<TransformComponent>(m_MaxComponentCount, this);

	f_SceneLoadingFinishCallback = [&]() {
		SortTransformComponentsVector();

		auto l_component = g_Engine->Get<ComponentManager>()->GetAll<TransformComponent>();

		for (auto i : l_component)
		{
			i->m_localTransformVector_target = i->m_localTransformVector;

			if (i->m_parentTransformComponent)
			{
				i->m_globalTransformVector = Math::LocalTransformVectorToGlobal(i->m_localTransformVector, i->m_parentTransformComponent->m_globalTransformVector, i->m_parentTransformComponent->m_globalTransformMatrix);
				i->m_globalTransformMatrix = Math::TransformVectorToTransformMatrix(i->m_globalTransformVector);
			}
		}
		};

	g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_SceneLoadingFinishCallback, 1);

	m_RootTransformEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Persistence, "RootTransform/");

	m_RootTransformComponent = g_Engine->Get<ComponentManager>()->Spawn<TransformComponent>(m_RootTransformEntity, false, ObjectLifespan::Persistence);

	m_RootTransformComponent->m_localTransformVector_target = m_RootTransformComponent->m_localTransformVector;
	m_RootTransformComponent->m_globalTransformVector = m_RootTransformComponent->m_localTransformVector;
	m_RootTransformComponent->m_globalTransformMatrix = Math::TransformVectorToTransformMatrix(m_RootTransformComponent->m_globalTransformVector);

	return true;
}

bool TransformSystem::Initialize()
{
	return true;
}

bool TransformSystem::Update()
{
	if (g_Engine->Get<SceneService>()->IsLoading())
		return true;

	auto l_component = g_Engine->Get<ComponentManager>()->GetAll<TransformComponent>();

	std::for_each(l_component.begin(), l_component.end(), [&](TransformComponent* val)
		{
			val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
		});

	SimulateTransformComponents();

	return true;
}

bool TransformSystem::Terminate()
{
	return true;
}

ObjectStatus TransformSystem::GetStatus()
{
	return ObjectStatus();
}

const TransformComponent* TransformSystem::GetRootTransformComponent()
{
	return m_RootTransformComponent;
}
