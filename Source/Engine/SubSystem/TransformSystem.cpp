#include "TransformSystem.h"
#include "../Component/TransformComponent.h"
#include "../Core/InnoRandomizer.h"
#include "../Core/InnoLogger.h"

#include "../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

namespace TransformSystemNS
{
	const size_t m_MaxComponentCount = 32768;
	InnoEntity* m_RootTransformEntity;
	TransformComponent* m_RootTransformComponent;

	std::function<void()> f_SceneLoadingFinishCallback;

	void SortTransformComponentsVector()
	{
		auto l_component = g_Engine->getComponentManager()->GetAll<TransformComponent>();

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
		l_ratio = InnoMath::clamp(l_ratio, 0.01f, 0.99f);

		auto l_component = g_Engine->getComponentManager()->GetAll<TransformComponent>();

		std::for_each(l_component.begin(), l_component.end(), [&](TransformComponent* val)
			{
				if (!InnoMath::isCloseEnough<float, 4>(val->m_localTransformVector.m_pos, val->m_localTransformVector_target.m_pos))
				{
					val->m_localTransformVector.m_pos = InnoMath::lerp(val->m_localTransformVector.m_pos, val->m_localTransformVector_target.m_pos, l_ratio);
				}

				if (!InnoMath::isCloseEnough<float, 4>(val->m_localTransformVector.m_rot, val->m_localTransformVector_target.m_rot))
				{
					val->m_localTransformVector.m_rot = InnoMath::slerp(val->m_localTransformVector.m_rot, val->m_localTransformVector_target.m_rot, l_ratio);
				}

				if (!InnoMath::isCloseEnough<float, 4>(val->m_localTransformVector.m_scale, val->m_localTransformVector_target.m_scale))
				{
					val->m_localTransformVector.m_scale = InnoMath::lerp(val->m_localTransformVector.m_scale, val->m_localTransformVector_target.m_scale, l_ratio);
				}

				if (val->m_parentTransformComponent)
				{
					val->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(val->m_localTransformVector, val->m_parentTransformComponent->m_globalTransformVector, val->m_parentTransformComponent->m_globalTransformMatrix);
					val->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(val->m_globalTransformVector);
				}
			});
	}
}

using namespace TransformSystemNS;

bool InnoTransformSystem::Setup(ISystemConfig* systemConfig)
{
	g_Engine->getComponentManager()->RegisterType<TransformComponent>(m_MaxComponentCount, this);

	f_SceneLoadingFinishCallback = [&]() {
		SortTransformComponentsVector();

		auto l_component = g_Engine->getComponentManager()->GetAll<TransformComponent>();

		for (auto i : l_component)
		{
			i->m_localTransformVector_target = i->m_localTransformVector;

			if (i->m_parentTransformComponent)
			{
				i->m_globalTransformVector = InnoMath::LocalTransformVectorToGlobal(i->m_localTransformVector, i->m_parentTransformComponent->m_globalTransformVector, i->m_parentTransformComponent->m_globalTransformMatrix);
				i->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(i->m_globalTransformVector);
			}
		}
	};

	g_Engine->getSceneSystem()->addSceneLoadingFinishCallback(&f_SceneLoadingFinishCallback);

	m_RootTransformEntity = g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Persistence, "RootTransform/");

	m_RootTransformComponent = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_RootTransformEntity, false, ObjectLifespan::Persistence);

	m_RootTransformComponent->m_localTransformVector_target = m_RootTransformComponent->m_localTransformVector;
	m_RootTransformComponent->m_globalTransformVector = m_RootTransformComponent->m_localTransformVector;
	m_RootTransformComponent->m_globalTransformMatrix = InnoMath::TransformVectorToTransformMatrix(m_RootTransformComponent->m_globalTransformVector);

	return true;
}

bool InnoTransformSystem::Initialize()
{
	return true;
}

bool InnoTransformSystem::Update()
{
	auto l_SimulateTask = g_Engine->getTaskSystem()->Submit("TransformComponentsSimulateTask", 0, nullptr, [&]()
		{
			SimulateTransformComponents();
		});

	return true;
}

bool InnoTransformSystem::OnFrameEnd()
{
	auto l_component = g_Engine->getComponentManager()->GetAll<TransformComponent>();

	std::for_each(l_component.begin(), l_component.end(), [&](TransformComponent* val)
		{
			val->m_globalTransformMatrix_prev = val->m_globalTransformMatrix;
		});
	return true;
}

bool InnoTransformSystem::Terminate()
{
	return true;
}

ObjectStatus InnoTransformSystem::GetStatus()
{
	return ObjectStatus();
}