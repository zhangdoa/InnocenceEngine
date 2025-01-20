#include "../../Engine/Services/PhysicsSystem.h"
#include "../../Engine/Services/HIDService.h"

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

        Entity* m_playerCharacterEntity = nullptr;
        TransformComponent* m_playerTransformComponent = nullptr;
        ModelComponent* m_playerModelComponent = nullptr;

        Entity* m_playerCameraEntity = nullptr;
        TransformComponent* m_playerCameraTransformComponent = nullptr;
        CameraComponent* m_playerCameraComponent = nullptr;

        Entity* m_debugCameraEntity = nullptr;
        TransformComponent* m_debugCameraTransformComponent = nullptr;
        CameraComponent* m_debugCameraComponent = nullptr;

        TransformComponent* m_activeCameraTransformComponent = nullptr;
        CameraComponent* m_activeCameraComponent = nullptr;

        AnimationController* m_animationController = nullptr;

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

        float m_initialMoveSpeed = 0;
        float m_moveSpeed = 0;
        float m_rotateSpeed = 0;
        bool m_canMove = false;
        bool m_canSlerp = false;

#ifdef EDITOR_MODE
        bool m_smoothInterp = false;
        bool m_isTP = false;
#else
        bool m_smoothInterp = true;
        bool m_isTP = true;
#endif

        bool m_isEventsRegistered = false;
        void Move(TransformComponent* transformComponent, Direction direction, float length);
        void RotateAroundPositiveYAxis(float offset);
        void RotateAroundRightAxis(float offset);

        Vec4 m_targetCameraRotX;
        Vec4 m_targetCameraRotY;
        Vec4 m_cameraPlayerDistance;
    };

    bool Player::Setup()
    {
        auto l_rootTransformComponent = g_Engine->Get<ComponentManager>()->Get<TransformComponent>(0);
        auto l_playerCharacterEntity = g_Engine->Get<EntityManager>()->Find("playerCharacter");
        if (l_playerCharacterEntity.has_value())
        {
            m_playerCharacterEntity = *l_playerCharacterEntity;
            m_playerTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(m_playerCharacterEntity);
            m_playerModelComponent = g_Engine->Get<ComponentManager>()->Find<ModelComponent>(m_playerCharacterEntity);
        }

        auto l_playerCameraEntity = g_Engine->Get<EntityManager>()->Find("playerCharacterCamera");
        if (l_playerCameraEntity.has_value())
        {
            m_playerCameraEntity = *l_playerCameraEntity;
            m_playerCameraComponent = g_Engine->Get<ComponentManager>()->Find<CameraComponent>(m_playerCameraEntity);
            m_playerCameraTransformComponent = g_Engine->Get<ComponentManager>()->Find<TransformComponent>(m_playerCameraEntity);
        }
        else
        {
            m_playerCameraEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Scene, "playerCharacterCamera/");
            m_playerCameraComponent = g_Engine->Get<ComponentManager>()->Spawn<CameraComponent>(m_playerCameraEntity, false, ObjectLifespan::Scene);
            m_playerCameraTransformComponent = g_Engine->Get<ComponentManager>()->Spawn<TransformComponent>(m_playerCameraEntity, false, ObjectLifespan::Scene);
            m_playerCameraTransformComponent->m_parentTransformComponent = l_rootTransformComponent;
        }

        if (!m_debugCameraEntity)
        {
            m_debugCameraEntity = g_Engine->Get<EntityManager>()->Spawn(false, ObjectLifespan::Persistence, "debugCamera/");
            m_debugCameraComponent = g_Engine->Get<ComponentManager>()->Spawn<CameraComponent>(m_debugCameraEntity, false, ObjectLifespan::Persistence);
            m_debugCameraTransformComponent = g_Engine->Get<ComponentManager>()->Spawn<TransformComponent>(m_debugCameraEntity, false, ObjectLifespan::Persistence);
            m_debugCameraTransformComponent->m_parentTransformComponent = l_rootTransformComponent;

            m_debugCameraComponent->m_FOVX = m_playerCameraComponent->m_FOVX;
            m_debugCameraComponent->m_zNear = m_playerCameraComponent->m_zNear;
            m_debugCameraComponent->m_zFar = m_playerCameraComponent->m_zFar;
            m_debugCameraComponent->m_widthScale = m_playerCameraComponent->m_widthScale;
            m_debugCameraComponent->m_heightScale = m_playerCameraComponent->m_heightScale;
        }

        m_activeCameraTransformComponent = m_playerCameraTransformComponent;
        m_activeCameraComponent = m_playerCameraComponent;
        static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->SetMainCamera(m_playerCameraComponent);
        static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->SetActiveCamera(m_activeCameraComponent);

        m_targetCameraRotX = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_targetCameraRotY = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_cameraPlayerDistance = m_playerCameraTransformComponent->m_localTransformVector.m_pos - m_playerTransformComponent->m_localTransformVector.m_pos;
        m_initialMoveSpeed = 0.05f;
        m_moveSpeed = m_initialMoveSpeed;
        m_rotateSpeed = 10.0f;

        if (!m_animationController)
            m_animationController = new AnimationController();
        m_animationController->Setup();

        if(m_isEventsRegistered)
            return true;

        // -z actually so Direction::Backward
        f_moveForward = [&]() {
            auto l_tickTime = g_Engine->getTickTime();
            l_tickTime;
            auto l_moveSpeed = m_moveSpeed * l_tickTime;

            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Backward, l_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Backward, l_moveSpeed);
            }
        };
        // +z actually so Direction::Backward
        f_moveBackward = [&]() {
            auto l_tickTime = g_Engine->getTickTime();
            l_tickTime;
            auto l_moveSpeed = m_moveSpeed * l_tickTime;            
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Forward, l_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Forward, l_moveSpeed);
            }
        };
        f_moveLeft = [&]() {
            auto l_tickTime = g_Engine->getTickTime();
            l_tickTime;
            auto l_moveSpeed = m_moveSpeed * l_tickTime;             
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Left, l_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Left, l_moveSpeed);
            }
        };
        f_moveRight = [&]() {
            auto l_tickTime = g_Engine->getTickTime();
            l_tickTime;
            auto l_moveSpeed = m_moveSpeed * l_tickTime;   
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Right, l_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Right, l_moveSpeed);
            }
        };

        f_move = [&]() { m_animationController->ChangeState("Run"); };
        f_stop = [&]() { m_animationController->ChangeState("Idle"); };

        f_speedUp = [&]() { m_moveSpeed = m_initialMoveSpeed * 10.0f; };
        f_speedDown = [&]() { m_moveSpeed = m_initialMoveSpeed; };

        f_allowMove = [&]() { m_canMove = true; };
        f_forbidMove = [&]() { m_canMove = false; };

        f_rotateAroundPositiveYAxis = std::bind(&Player::RotateAroundPositiveYAxis, this, std::placeholders::_1);
        f_rotateAroundRightAxis = std::bind(&Player::RotateAroundRightAxis, this, std::placeholders::_1);

        f_addForce = [&]() {
            auto l_force = Math::getDirection(Direction::Backward, m_playerCameraTransformComponent->m_localTransformVector.m_rot);
            l_force = l_force * 10.0f;
            g_Engine->Get<PhysicsSystem>()->AddForce(m_playerModelComponent, l_force);
        };

        f_switchCamera = [&]() {
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                m_activeCameraComponent = m_debugCameraComponent;
                m_activeCameraTransformComponent = m_debugCameraTransformComponent;
            }
            else
            {
                m_activeCameraComponent = m_playerCameraComponent;
                m_activeCameraTransformComponent = m_playerCameraTransformComponent;
            }

            static_cast<ICameraSystem*>(g_Engine->Get<ComponentManager>()->GetComponentSystem<CameraComponent>())->SetActiveCamera(m_activeCameraComponent);
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
        
        m_isEventsRegistered = true;
        
        return true;
    }

    bool Player::Initialize()
    {
        return true;
    }

    void Player::Move(TransformComponent* transformComponent, Direction direction, float length)
    {
        if (m_canMove)
        {
            auto l_dir = Math::getDirection(direction, transformComponent->m_localTransformVector.m_rot);
            auto l_currentPos = transformComponent->m_localTransformVector.m_pos;
            transformComponent->m_localTransformVector_target.m_pos = Math::moveTo(l_currentPos, l_dir, length);
            if (!m_smoothInterp)
            {
                transformComponent->m_localTransformVector.m_pos = transformComponent->m_localTransformVector_target.m_pos;
            }
        }
    }

    void Player::RotateAroundPositiveYAxis(float offset)
    {
        if (m_canMove)
        {
            m_targetCameraRotY = Math::getQuatRotator(
                Vec4(0.0f, 1.0f, 0.0f, 0.0f),
                ((-offset * m_rotateSpeed) / 180.0f) * PI<float>);

            m_canSlerp = false;

            m_playerTransformComponent->m_localTransformVector_target.m_rot = m_targetCameraRotY.quatMul(m_playerTransformComponent->m_localTransformVector_target.m_rot);
            m_activeCameraTransformComponent->m_localTransformVector_target.m_rot = m_targetCameraRotY.quatMul(m_activeCameraTransformComponent->m_localTransformVector_target.m_rot);

            if (!m_smoothInterp)
            {
                m_playerTransformComponent->m_localTransformVector.m_rot = m_playerTransformComponent->m_localTransformVector_target.m_rot;
                m_activeCameraTransformComponent->m_localTransformVector.m_rot = m_activeCameraTransformComponent->m_localTransformVector_target.m_rot;
            }
            m_canSlerp = true;
        }
    }

    void Player::RotateAroundRightAxis(float offset)
    {
        if (m_canMove)
        {
            m_canSlerp = false;

            auto l_right = Math::getDirection(Direction::Right, m_activeCameraTransformComponent->m_localTransformVector_target.m_rot);
            m_targetCameraRotX = Math::getQuatRotator(
                l_right,
                ((offset * m_rotateSpeed) / 180.0f) * PI<float>);
            m_activeCameraTransformComponent->m_localTransformVector_target.m_rot = m_targetCameraRotX.quatMul(m_activeCameraTransformComponent->m_localTransformVector_target.m_rot);

            if (!m_smoothInterp)
            {
                m_activeCameraTransformComponent->m_localTransformVector.m_rot = m_activeCameraTransformComponent->m_localTransformVector_target.m_rot;
            }

            m_canSlerp = true;
        }
    }

    bool Player::Update(float seed)
    {
        if (m_animationController)
        {
            m_animationController->Simulate();
        }

        if (m_isTP)
        {
            auto l_t = m_playerTransformComponent->m_globalTransformMatrix.m_translationMat;
            auto l_r = m_playerTransformComponent->m_globalTransformMatrix.m_rotationMat;
            auto l_m = l_t * l_r;

            auto l_lp = m_cameraPlayerDistance;
            m_cameraPlayerDistance.w = 1.0f;
            auto l_gp = Math::mul(l_m, m_cameraPlayerDistance);

            m_playerCameraTransformComponent->m_localTransformVector_target.m_pos = l_gp;
            m_playerCameraTransformComponent->m_localTransformVector.m_pos = l_gp;
        }

        // auto l_targetCameraRotY = Math::getQuatRotator(
        //     Vec4(1.0f, 0.0f, 0.0f, 0.0f),
        //     ((-30.0f) / 180.0f) * PI<float>);

        // m_playerCameraTransformComponent->m_localTransformVector_target.m_rot = l_targetCameraRotY.quatMul(m_playerCameraTransformComponent->m_localTransformVector_target.m_rot);

        return true;
    }

    bool Player::Terminate()
    {
        if (m_animationController)
        {
            delete m_animationController;
            return true;
        }

        return false;
    }
}