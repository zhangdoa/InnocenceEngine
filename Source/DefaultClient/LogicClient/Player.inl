#include "../../Engine/Services/PhysicsSimulationService.h"
#include "../../Engine/Services/HIDService.h"
#include "../../Engine/Services/EntityRegistry.h"
#include "../../Engine/Services/CameraSystem.h"
#include "../../Engine/Component/TransformComponent.h"
#include "../../Engine/Component/CameraComponent.h"

#include "../../Engine/Engine.h"
using namespace Inno;
;

#include "AnimationController.inl"

#define EDITOR_MODE

namespace Inno
{
	class Player
	{
	public:
		bool Setup();
		bool Initialize();
		bool Update(float seed);
		bool Terminate();
		void OnSceneLoadingFinished();

		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

		EntityID m_PlayerCharacterEntity = INVALID_ENTITY;

		EntityID m_PlayerCameraEntity = INVALID_ENTITY;
		CameraComponent* m_PlayerCameraComponent = nullptr;

		EntityID m_DebugCameraEntity = INVALID_ENTITY;
		CameraComponent* m_DebugCameraComponent = nullptr;

		CameraComponent* m_ActiveCameraComponent = nullptr;

		AnimationController* m_AnimationController = nullptr;

		std::function<void()> f_switchCamera;

		std::function<void()> f_moveForward;
		std::function<void()> f_moveBackward;
		std::function<void()> f_moveLeft;
		std::function<void()> f_moveRight;
		std::function<void()> f_move;
		std::function<void()> f_stop;

		std::function<void()> f_allowMove;
		std::function<void()> f_forbidMove;

		std::function<void()> f_speedUp;
		std::function<void()> f_speedDown;

		std::function<void(float)> f_rotateAroundPositiveYAxis;
		std::function<void(float)> f_rotateAroundRightAxis;

		std::function<void()> f_addForce;

		float m_InitialMoveSpeed = 0;
		float m_MoveSpeed = 0;
		float m_RotateSpeed = 0;
		bool m_CanMove = false;
		bool m_CanSlerp = false;

#ifdef EDITOR_MODE
		bool m_SmoothInterp = false;
		bool m_IsTP = false;
#else
		bool m_SmoothInterp = true;
		bool m_IsTP = true;
#endif

		bool m_IsEventsRegistered = false;
		void MoveCamera(EntityID CameraEntity, Direction direction, float length);
		void MoveModel(EntityID Entity, Direction direction, float length);
		void RotateAroundPositiveYAxis(float offset);
		void RotateAroundRightAxis(float offset);

		Vec4 m_TargetCameraRotX;
		Vec4 m_TargetCameraRotY;
		Vec4 m_CameraPlayerDistance;
	};

	bool Player::Setup()
	{
		auto l_Registry = g_Engine->Get<EntityRegistry>();

		auto l_PlayerCharacterEntity = l_Registry->FindByName("Player Character");
		if (l_PlayerCharacterEntity != INVALID_ENTITY)
		{
			m_PlayerCharacterEntity = l_PlayerCharacterEntity;
		}
		else
		{
			m_PlayerCharacterEntity = l_Registry->Spawn(ObjectLifespan::Scene, "Player Character/");
		}

		auto l_PlayerCameraEntity = l_Registry->FindByName("Main Camera");
		if (l_PlayerCameraEntity != INVALID_ENTITY)
		{
			m_PlayerCameraEntity = l_PlayerCameraEntity;
			m_PlayerCameraComponent = l_Registry->Get<CameraComponent>(m_PlayerCameraEntity);
		}
		else
		{
			m_PlayerCameraEntity = l_Registry->Spawn(ObjectLifespan::Scene, "Main Camera/");
			auto& l_Camera = l_Registry->Emplace<CameraComponent>(m_PlayerCameraEntity);
			l_Registry->Emplace<TransformComponent>(m_PlayerCameraEntity);
			m_PlayerCameraComponent = &l_Camera;
		}

		if (m_DebugCameraEntity == INVALID_ENTITY)
		{
			m_DebugCameraEntity = l_Registry->Spawn(ObjectLifespan::Persistence, "Debug Camera/");
			auto& l_DebugCamera = l_Registry->Emplace<CameraComponent>(m_DebugCameraEntity);
			l_Registry->Emplace<TransformComponent>(m_DebugCameraEntity);
			m_DebugCameraComponent = &l_DebugCamera;

			m_DebugCameraComponent->m_FOVX = m_PlayerCameraComponent->m_FOVX;
			m_DebugCameraComponent->m_ZNear = m_PlayerCameraComponent->m_ZNear;
			m_DebugCameraComponent->m_ZFar = m_PlayerCameraComponent->m_ZFar;
			m_DebugCameraComponent->m_WidthScale = m_PlayerCameraComponent->m_WidthScale;
			m_DebugCameraComponent->m_HeightScale = m_PlayerCameraComponent->m_HeightScale;
		}

		m_ActiveCameraComponent = m_PlayerCameraComponent;
		g_Engine->Get<CameraSystem>()->SetMainCamera(m_PlayerCameraComponent);
		g_Engine->Get<CameraSystem>()->SetActiveCamera(m_ActiveCameraComponent);

		m_TargetCameraRotX = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		m_TargetCameraRotY = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		// TODO Phase2-migrate: compute camera-player distance from TransformComponents
		m_InitialMoveSpeed = 0.05f;
		m_MoveSpeed = m_InitialMoveSpeed;
		m_RotateSpeed = 10.0f;

		if (!m_AnimationController)
			m_AnimationController = new AnimationController();
		m_AnimationController->Setup();

		if(m_IsEventsRegistered)
			return true;

		// -z actually so Direction::Backward
		f_moveForward = [&]() {
			auto l_TickTime = g_Engine->getTickTime();
			auto l_MoveSpd = m_MoveSpeed * l_TickTime;

			if (m_ActiveCameraComponent == m_PlayerCameraComponent)
			{
				MoveModel(m_PlayerCharacterEntity, Direction::Backward, l_MoveSpd);
			}
			if (!m_IsTP)
			{
				// TODO Phase2-migrate: need EntityID for active camera to call MoveCamera
			}
		};
		// +z actually so Direction::Backward
		f_moveBackward = [&]() {
			auto l_TickTime = g_Engine->getTickTime();
			auto l_MoveSpd = m_MoveSpeed * l_TickTime;
			if (m_ActiveCameraComponent == m_PlayerCameraComponent)
			{
				MoveModel(m_PlayerCharacterEntity, Direction::Forward, l_MoveSpd);
			}
			if (!m_IsTP)
			{
				// TODO Phase2-migrate: need EntityID for active camera to call MoveCamera
			}
		};
		f_moveLeft = [&]() {
			auto l_TickTime = g_Engine->getTickTime();
			auto l_MoveSpd = m_MoveSpeed * l_TickTime;
			if (m_ActiveCameraComponent == m_PlayerCameraComponent)
			{
				MoveModel(m_PlayerCharacterEntity, Direction::Left, l_MoveSpd);
			}
			if (!m_IsTP)
			{
				// TODO Phase2-migrate: need EntityID for active camera to call MoveCamera
			}
		};
		f_moveRight = [&]() {
			auto l_TickTime = g_Engine->getTickTime();
			auto l_MoveSpd = m_MoveSpeed * l_TickTime;
			if (m_ActiveCameraComponent == m_PlayerCameraComponent)
			{
				MoveModel(m_PlayerCharacterEntity, Direction::Right, l_MoveSpd);
			}
			if (!m_IsTP)
			{
				// TODO Phase2-migrate: need EntityID for active camera to call MoveCamera
			}
		};

		f_move = [&]() { m_AnimationController->ChangeState("Run"); };
		f_stop = [&]() { m_AnimationController->ChangeState("Idle"); };

		f_speedUp = [&]() { m_MoveSpeed = m_InitialMoveSpeed * 10.0f; };
		f_speedDown = [&]() { m_MoveSpeed = m_InitialMoveSpeed; };

		f_allowMove = [&]() { m_CanMove = true; };
		f_forbidMove = [&]() { m_CanMove = false; };

		f_rotateAroundPositiveYAxis = std::bind(&Player::RotateAroundPositiveYAxis, this, std::placeholders::_1);
		f_rotateAroundRightAxis = std::bind(&Player::RotateAroundRightAxis, this, std::placeholders::_1);

		f_addForce = [&]() {
			// TODO Phase2-migrate: get camera TransformComponent for direction
			auto l_Force = Vec4(0.0f, 0.0f, -1.0f, 0.0f);
			l_Force = l_Force * 10.0f;
			g_Engine->Get<PhysicsSimulationService>()->AddForce(m_PlayerCharacterEntity, l_Force);
		};

		f_switchCamera = [&]() {
			if (m_ActiveCameraComponent == m_PlayerCameraComponent)
			{
				m_ActiveCameraComponent = m_DebugCameraComponent;
			}
			else
			{
				m_ActiveCameraComponent = m_PlayerCameraComponent;
			}

			g_Engine->Get<CameraSystem>()->SetActiveCamera(m_ActiveCameraComponent);
		};

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_W, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveForward });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_S, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveBackward });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_A, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveLeft });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_D, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveRight });

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_W, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_S, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_A, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_D, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_W, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_S, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_A, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_D, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_E, true }, ButtonEvent{ EventLifeTime::OneShot, &f_addForce });

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_SPACE, true }, ButtonEvent{ EventLifeTime::Continuous, &f_speedUp });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_SPACE, false }, ButtonEvent{ EventLifeTime::Continuous, &f_speedDown });

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_MOUSE_BUTTON_RIGHT, true }, ButtonEvent{ EventLifeTime::Continuous, &f_allowMove });
		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_MOUSE_BUTTON_RIGHT, false }, ButtonEvent{ EventLifeTime::Continuous, &f_forbidMove });
		g_Engine->Get<HIDService>()->AddMouseMovementCallback(MouseMovementAxis::Horizontal, MouseMovementEvent{ EventLifeTime::OneShot, &f_rotateAroundPositiveYAxis });
		g_Engine->Get<HIDService>()->AddMouseMovementCallback(MouseMovementAxis::Vertical, MouseMovementEvent{ EventLifeTime::OneShot, &f_rotateAroundRightAxis });

		g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_O, true }, ButtonEvent{ EventLifeTime::OneShot, &f_switchCamera });

		m_IsEventsRegistered = true;

		return true;
	}

	bool Player::Initialize()
	{
		return true;
	}

	void Player::MoveCamera(EntityID CameraEntity, Direction direction, float length)
	{
		if (m_CanMove)
		{
			auto* l_Transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(CameraEntity);
			if (l_Transform)
			{
				auto l_Dir = Math::getDirection(direction, l_Transform->m_LocalRot);
				l_Transform->m_LocalPos = Math::moveTo<float>(Vec4(l_Transform->m_LocalPos, 1.0f), l_Dir, length).xyz();
			}
		}
	}

	void Player::MoveModel(EntityID Entity, Direction direction, float length)
	{
		if (m_CanMove)
		{
			if (Entity == INVALID_ENTITY)
				return;
			auto* l_transform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(Entity);
			if (!l_transform)
				return;
			auto l_Dir = Math::getDirection(direction, Vec4(l_transform->m_LocalRot.x, l_transform->m_LocalRot.y, l_transform->m_LocalRot.z, l_transform->m_LocalRot.w));
			l_transform->m_LocalPos = Math::moveTo<float>(Vec4(l_transform->m_LocalPos, 1.0f), l_Dir, length).xyz();
		}
	}

	void Player::RotateAroundPositiveYAxis(float offset)
	{
		if (m_CanMove)
		{
			m_TargetCameraRotY = Math::getQuatRotator(
				Vec4(0.0f, 1.0f, 0.0f, 0.0f),
				((-offset * m_RotateSpeed) / 180.0f) * PI<float>);

			m_CanSlerp = false;

			auto* l_playerTransform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(m_PlayerCharacterEntity);
			if (l_playerTransform)
			{
				l_playerTransform->m_LocalRot = m_TargetCameraRotY.quatMul(l_playerTransform->m_LocalRot);
			}
			// TODO Phase2-migrate: rotate active camera TransformComponent

			m_CanSlerp = true;
		}
	}

	void Player::RotateAroundRightAxis(float offset)
	{
		if (m_CanMove)
		{
			m_CanSlerp = false;

			// TODO Phase2-migrate: get right direction from active camera TransformComponent
			auto l_Right = Vec4(1.0f, 0.0f, 0.0f, 0.0f);
			m_TargetCameraRotX = Math::getQuatRotator(
				l_Right,
				((offset * m_RotateSpeed) / 180.0f) * PI<float>);
			// TODO Phase2-migrate: rotate active camera TransformComponent

			m_CanSlerp = true;
		}
	}

	bool Player::Update(float seed)
	{
		if (m_AnimationController)
		{
			m_AnimationController->Simulate();
		}

		auto* l_playerTransform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(m_PlayerCharacterEntity);
		if (m_IsTP && l_playerTransform)
		{
			auto l_T = Vec4(l_playerTransform->m_LocalPos, 1.0f);
			auto l_R = l_playerTransform->m_LocalRot;

			auto l_LP = m_CameraPlayerDistance;
			m_CameraPlayerDistance.w = 1.0f;
			auto l_GP = l_T + (Math::toRotationMatrix(l_R) * m_CameraPlayerDistance).xyz();

			auto* l_CameraTransform = g_Engine->Get<EntityRegistry>()->Get<TransformComponent>(m_PlayerCameraEntity);
			if (l_CameraTransform)
			{
				l_CameraTransform->m_LocalPos = Vec3(l_GP.x, l_GP.y, l_GP.z);
			}
		}

		return true;
	}

	bool Player::Terminate()
	{
		if (m_AnimationController)
		{
			delete m_AnimationController;
			return true;
		}

		return false;
	}
}
