#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

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

        InnoEntity* m_playerCharacterEntity = nullptr;
        TransformComponent* m_playerTransformComponent = nullptr;
        VisibleComponent* m_playerVisibleComponent = nullptr;

        InnoEntity* m_playerCameraEntity = nullptr;
        TransformComponent* m_playerCameraTransformComponent = nullptr;
        CameraComponent* m_playerCameraComponent = nullptr;

        InnoEntity* m_debugCameraEntity = nullptr;
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
        auto l_rootTransformComponent = g_Engine->getComponentManager()->Get<TransformComponent>(0);
        auto l_playerCharacterEntity = g_Engine->getEntityManager()->Find("playerCharacter");
        if (l_playerCharacterEntity.has_value())
        {
            m_playerCharacterEntity = *l_playerCharacterEntity;
            m_playerTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(m_playerCharacterEntity);
            m_playerVisibleComponent = g_Engine->getComponentManager()->Find<VisibleComponent>(m_playerCharacterEntity);
        }

        auto l_playerCameraEntity = g_Engine->getEntityManager()->Find("playerCharacterCamera");
        if (l_playerCameraEntity.has_value())
        {
            m_playerCameraEntity = *l_playerCameraEntity;
            m_playerCameraComponent = g_Engine->getComponentManager()->Find<CameraComponent>(m_playerCameraEntity);
            m_playerCameraTransformComponent = g_Engine->getComponentManager()->Find<TransformComponent>(m_playerCameraEntity);
        }
        else
        {
            m_playerCameraEntity = g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Scene, "playerCharacterCamera/");
            m_playerCameraComponent = g_Engine->getComponentManager()->Spawn<CameraComponent>(m_playerCameraEntity, false, ObjectLifespan::Scene);
            m_playerCameraTransformComponent = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_playerCameraEntity, false, ObjectLifespan::Scene);
            m_playerCameraTransformComponent->m_parentTransformComponent = l_rootTransformComponent;
        }

        if (!m_debugCameraEntity)
        {
            m_debugCameraEntity = g_Engine->getEntityManager()->Spawn(false, ObjectLifespan::Persistence, "debugCamera/");
            m_debugCameraComponent = g_Engine->getComponentManager()->Spawn<CameraComponent>(m_debugCameraEntity, false, ObjectLifespan::Persistence);
            m_debugCameraTransformComponent = g_Engine->getComponentManager()->Spawn<TransformComponent>(m_debugCameraEntity, false, ObjectLifespan::Persistence);
            m_debugCameraTransformComponent->m_parentTransformComponent = l_rootTransformComponent;

            m_debugCameraComponent->m_FOVX = m_playerCameraComponent->m_FOVX;
            m_debugCameraComponent->m_zNear = m_playerCameraComponent->m_zNear;
            m_debugCameraComponent->m_zFar = m_playerCameraComponent->m_zFar;
            m_debugCameraComponent->m_widthScale = m_playerCameraComponent->m_widthScale;
            m_debugCameraComponent->m_heightScale = m_playerCameraComponent->m_heightScale;
        }

        m_activeCameraTransformComponent = m_playerCameraTransformComponent;
        m_activeCameraComponent = m_playerCameraComponent;
        static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->SetMainCamera(m_playerCameraComponent);
        static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->SetActiveCamera(m_activeCameraComponent);

        m_targetCameraRotX = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_targetCameraRotY = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
        m_cameraPlayerDistance = m_playerCameraTransformComponent->m_localTransformVector.m_pos - m_playerTransformComponent->m_localTransformVector.m_pos;
        m_initialMoveSpeed = 0.5f;
        m_moveSpeed = m_initialMoveSpeed;
        m_rotateSpeed = 10.0f;

        if (!m_animationController)
            m_animationController = new AnimationController();
        m_animationController->Setup();

        if(m_isEventsRegistered)
            return true;

        // -z actually so Direction::Backward
        f_moveForward = [&]() {
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Backward, m_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Backward, m_moveSpeed);
            }
        };
        // +z actually so Direction::Backward
        f_moveBackward = [&]() {
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Forward, m_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Forward, m_moveSpeed);
            }
        };
        f_moveLeft = [&]() {
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Left, m_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Left, m_moveSpeed);
            }
        };
        f_moveRight = [&]() {
            if (m_activeCameraTransformComponent == m_playerCameraTransformComponent)
            {
                Move(m_playerTransformComponent, Direction::Right, m_moveSpeed);
            }
            if (!m_isTP)
            {
                Move(m_activeCameraTransformComponent, Direction::Right, m_moveSpeed);
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
            auto l_force = InnoMath::getDirection(Direction::Backward, m_playerCameraTransformComponent->m_localTransformVector.m_rot);
            l_force = l_force * 10.0f;
            g_Engine->getPhysicsSystem()->addForce(m_playerVisibleComponent, l_force);
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

            static_cast<ICameraSystem*>(g_Engine->getComponentManager()->GetComponentSystem<CameraComponent>())->SetActiveCamera(m_activeCameraComponent);
        };

        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_W, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveForward });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_S, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveBackward });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_A, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveLeft });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_D, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveRight });

        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_W, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_S, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_A, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_D, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });

        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_W, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_S, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_A, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_D, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });

        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_E, true }, ButtonEvent{ EventLifeTime::OneShot, &f_addForce });

        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_SPACE, true }, ButtonEvent{ EventLifeTime::Continuous, &f_speedUp });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_SPACE, false }, ButtonEvent{ EventLifeTime::Continuous, &f_speedDown });

        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_MOUSE_BUTTON_RIGHT, true }, ButtonEvent{ EventLifeTime::Continuous, &f_allowMove });
        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_MOUSE_BUTTON_RIGHT, false }, ButtonEvent{ EventLifeTime::Continuous, &f_forbidMove });
        g_Engine->getEventSystem()->addMouseMovementCallback(MouseMovementAxis::Horizontal, MouseMovementEvent{ EventLifeTime::OneShot, &f_rotateAroundPositiveYAxis });
        g_Engine->getEventSystem()->addMouseMovementCallback(MouseMovementAxis::Vertical, MouseMovementEvent{ EventLifeTime::OneShot, &f_rotateAroundRightAxis });

        g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_O, true }, ButtonEvent{ EventLifeTime::OneShot, &f_switchCamera });
        
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
            auto l_dir = InnoMath::getDirection(direction, transformComponent->m_localTransformVector.m_rot);
            auto l_currentPos = transformComponent->m_localTransformVector.m_pos;
            transformComponent->m_localTransformVector_target.m_pos = InnoMath::moveTo(l_currentPos, l_dir, length);
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
            m_targetCameraRotY = InnoMath::getQuatRotator(
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

            auto l_right = InnoMath::getDirection(Direction::Right, m_activeCameraTransformComponent->m_localTransformVector_target.m_rot);
            m_targetCameraRotX = InnoMath::getQuatRotator(
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
            auto l_gp = InnoMath::mul(l_m, m_cameraPlayerDistance);

            m_playerCameraTransformComponent->m_localTransformVector_target.m_pos = l_gp;
            m_playerCameraTransformComponent->m_localTransformVector.m_pos = l_gp;
        }

        auto l_targetCameraRotY = InnoMath::getQuatRotator(
            Vec4(1.0f, 0.0f, 0.0f, 0.0f),
            ((-30.0f) / 180.0f) * PI<float>);

        m_playerCameraTransformComponent->m_localTransformVector_target.m_rot = l_targetCameraRotY.quatMul(m_playerCameraTransformComponent->m_localTransformVector_target.m_rot);

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