
#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/AnimationSimulationService.h"

#include "../../Engine/Engine.h"

using namespace Inno;


namespace Inno
{
	class AnimationController
	{
	public:
		bool Setup();
		bool Simulate();
		bool ChangeState(const std::string& state);

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		EntityID m_Entity = INVALID_ENTITY;
		std::string m_currentState;
		bool m_isStateChanged;

		std::unordered_map<std::string, std::function<void()>> m_states;
	};

	bool AnimationController::Setup()
	{
		auto l_Entity = g_Engine->Get<EntityRegistry>()->FindByName("playerCharacter");
		if (l_Entity != INVALID_ENTITY)
		{
			m_Entity = l_Entity;

			std::function<void()> f_idle = [&]() {
				g_Engine->Get<AnimationSimulationService>()->PlayAnimation(m_Entity, "ConvertedAssets//Wolf_Wolf_Skeleton-Wolf_Idle_.InnoAnimation/", true);
			};

			std::function<void()> f_run = [&]() {
				g_Engine->Get<AnimationSimulationService>()->PlayAnimation(m_Entity, "ConvertedAssets//Wolf_Wolf_Skeleton-Wolf_Run_Cycle_.InnoAnimation/", true);
			};

			m_states.emplace("Idle", f_idle);
			m_states.emplace("Run", f_run);

			m_currentState = "Idle";
			m_isStateChanged = false;

			m_ObjectStatus = ObjectStatus::Activated;
			return true;
		}

		return false;
	}

	bool AnimationController::Simulate()
	{
		if (m_ObjectStatus == ObjectStatus::Activated && m_isStateChanged)
		{
			auto l_func = m_states.find(m_currentState);
			if (l_func != m_states.end())
			{
				g_Engine->Get<AnimationSimulationService>()->StopAnimation(m_Entity);
				l_func->second();
				m_isStateChanged = false;

				return true;
			}
		}

		return false;
	}

	bool AnimationController::ChangeState(const std::string& state)
	{
		if (state != m_currentState)
		{
			m_currentState = state;
			m_isStateChanged = true;
			return true;
		}

		return false;
	}
}
