#include "DefaultLogicClient.h"
#include "../../Engine/Common/CommonMacro.inl"
#include "../../Engine/ComponentManager/ITransformComponentManager.h"
#include "../../Engine/ComponentManager/IVisibleComponentManager.h"
#include "../../Engine/ComponentManager/ILightComponentManager.h"
#include "../../Engine/ComponentManager/ICameraComponentManager.h"

#include "../../Engine/Interface/IModuleManager.h"
INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace AnimationStateMachine
{
	bool setup();
	bool simulate();
	bool changeState(const std::string& state);

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	InnoEntity* m_entity;
	VisibleComponent* m_visibleComponent;
	std::string m_currentState;
	bool isStateChanged;

	std::unordered_map<std::string, std::function<void()>> m_states;

	bool setup()
	{
		auto l_entity = g_pModuleManager->getEntityManager()->Find("playerCharacter");
		if (l_entity.has_value())
		{
			m_entity = *l_entity;
			m_visibleComponent = GetComponent(VisibleComponent, *l_entity);

			std::function<void()> f_idle = [&]() {
				g_pModuleManager->getRenderingFrontend()->playAnimation(m_visibleComponent, "..//Res//ConvertedAssets//Wolf_Wolf_Skeleton-Wolf_Idle_.InnoAnimation/", true);
			};

			std::function<void()> f_run = [&]() {
				g_pModuleManager->getRenderingFrontend()->playAnimation(m_visibleComponent, "..//Res//ConvertedAssets//Wolf_Wolf_Skeleton-Wolf_Run_Cycle_.InnoAnimation/", true);
			};

			m_states.emplace("Idle", f_idle);
			m_states.emplace("Run", f_run);

			m_currentState = "Idle";
			isStateChanged = false;

			m_ObjectStatus = ObjectStatus::Activated;
			return true;
		}

		return false;
	}

	bool simulate()
	{
		if (isStateChanged)
		{
			auto l_func = m_states.find(m_currentState);
			if (l_func != m_states.end())
			{
				g_pModuleManager->getRenderingFrontend()->stopAnimation(m_visibleComponent, "");
				l_func->second();
				isStateChanged = false;

				return true;
			}
		}
		return false;
	}

	bool changeState(const std::string& state)
	{
		if (state != m_currentState)
		{
			m_currentState = state;
			isStateChanged = true;
			return true;
		}
		return false;
	}
}

namespace PlayerComponentCollection
{
	bool setup();
	bool initialize();

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;

	InnoEntity* m_playerParentEntity;
	InnoEntity* m_playerCameraParentEntity;

	TransformComponent* m_playerTransformComponent;
	VisibleComponent* m_playerVisibleComponent;
	TransformComponent* m_playerCameraTransformComponent;

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
	bool m_smoothInterp = true;
	bool m_isTP = true;

	void move(Direction direction, float length);
	Vec4 m_targetCameraRotX;
	Vec4 m_targetCameraRotY;
	Vec4 m_cameraPlayerDistance;

	void update(float seed);

	void rotateAroundPositiveYAxis(float offset);
	void rotateAroundRightAxis(float offset);

	std::function<void()> f_sceneLoadingFinishCallback;
};

using namespace PlayerComponentCollection;

bool PlayerComponentCollection::setup()
{
	f_sceneLoadingFinishCallback = [&]() {
		auto l_playerParentEntity = g_pModuleManager->getEntityManager()->Find("playerCharacter");
		if (l_playerParentEntity.has_value())
		{
			m_playerParentEntity = *l_playerParentEntity;
			m_playerTransformComponent = GetComponent(TransformComponent, m_playerParentEntity);
			m_playerVisibleComponent = GetComponent(VisibleComponent, m_playerParentEntity);
		}

		auto l_playerCameraParentEntity = g_pModuleManager->getEntityManager()->Find("playerCharacterCamera");
		if (l_playerCameraParentEntity.has_value())
		{
			m_playerCameraParentEntity = *l_playerCameraParentEntity;
			m_playerCameraTransformComponent = GetComponent(TransformComponent, m_playerCameraParentEntity);
		}
		else
		{
			m_playerCameraParentEntity = g_pModuleManager->getEntityManager()->Spawn(false, ObjectLifespan::Scene, "playerCharacterCamera/");
			m_playerCameraTransformComponent = SpawnComponent(TransformComponent, m_playerParentEntity, false, ObjectLifespan::Scene);
		}

		m_targetCameraRotX = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		m_targetCameraRotY = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
		m_cameraPlayerDistance = m_playerCameraTransformComponent->m_localTransformVector.m_pos - m_playerTransformComponent->m_localTransformVector.m_pos;

		f_moveForward = [&]() { move(Direction::Forward, m_moveSpeed); };
		f_moveBackward = [&]() { move(Direction::Backward, m_moveSpeed); };
		f_moveLeft = [&]() { move(Direction::Left, m_moveSpeed); };
		f_moveRight = [&]() { move(Direction::Right, m_moveSpeed); };
		f_move = [&]() { AnimationStateMachine::changeState("Run"); };
		f_stop = [&]() { AnimationStateMachine::changeState("Idle"); };

		f_speedUp = [&]() { m_moveSpeed = m_initialMoveSpeed * 10.0f; };
		f_speedDown = [&]() { m_moveSpeed = m_initialMoveSpeed; };

		f_allowMove = [&]() { m_canMove = true; };
		f_forbidMove = [&]() { m_canMove = false; };

		f_rotateAroundPositiveYAxis = std::bind(&rotateAroundPositiveYAxis, std::placeholders::_1);
		f_rotateAroundRightAxis = std::bind(&rotateAroundRightAxis, std::placeholders::_1);

		f_addForce = [&]() {
			auto l_force = InnoMath::getDirection(Direction::Backward, m_playerTransformComponent->m_localTransformVector.m_rot);
			l_force = l_force * 10.0f;
			g_pModuleManager->getPhysicsSystem()->addForce(m_playerVisibleComponent, l_force);
		};

		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_S, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveForward });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_W, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveBackward });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_A, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveLeft });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_D, true }, ButtonEvent{ EventLifeTime::Continuous, &f_moveRight });

		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_S, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_W, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_A, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_D, true }, ButtonEvent{ EventLifeTime::OneShot, &f_move });

		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_S, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_W, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_A, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_D, false }, ButtonEvent{ EventLifeTime::OneShot, &f_stop });

		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_E, true }, ButtonEvent{ EventLifeTime::OneShot, &f_addForce });

		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_SPACE, true }, ButtonEvent{ EventLifeTime::Continuous, &f_speedUp });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_SPACE, false }, ButtonEvent{ EventLifeTime::Continuous, &f_speedDown });

		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_MOUSE_BUTTON_RIGHT, true }, ButtonEvent{ EventLifeTime::Continuous, &f_allowMove });
		g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_MOUSE_BUTTON_RIGHT, false }, ButtonEvent{ EventLifeTime::Continuous, &f_forbidMove });
		g_pModuleManager->getEventSystem()->addMouseMovementCallback(MouseMovementAxis::Horizontal, MouseMovementEvent{ EventLifeTime::OneShot, &f_rotateAroundPositiveYAxis });
		g_pModuleManager->getEventSystem()->addMouseMovementCallback(MouseMovementAxis::Vertical, MouseMovementEvent{ EventLifeTime::OneShot,&f_rotateAroundRightAxis });

		m_initialMoveSpeed = 0.5f;
		m_moveSpeed = m_initialMoveSpeed;
		m_rotateSpeed = 10.0f;

		AnimationStateMachine::setup();
	};
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback, 0);

	return true;
}

bool PlayerComponentCollection::initialize()
{
	return true;
}

void PlayerComponentCollection::move(Direction direction, float length)
{
	if (m_canMove)
	{
		if (m_isTP)
		{
			auto l_playerDir = InnoMath::getDirection(direction, m_playerTransformComponent->m_localTransformVector.m_rot);
			auto l_currentPlayerPos = m_playerTransformComponent->m_localTransformVector.m_pos;
			m_playerTransformComponent->m_localTransformVector_target.m_pos = InnoMath::moveTo(l_currentPlayerPos, l_playerDir, length);
		}
		else
		{
			auto l_cameraDir = InnoMath::getDirection(direction, m_playerCameraTransformComponent->m_localTransformVector.m_rot);
			auto l_currentCameraPos = m_playerCameraTransformComponent->m_localTransformVector.m_pos;
			m_playerCameraTransformComponent->m_localTransformVector_target.m_pos = InnoMath::moveTo(l_currentCameraPos, l_cameraDir, length);
		}
	}
}

void PlayerComponentCollection::rotateAroundPositiveYAxis(float offset)
{
	if (m_canMove)
	{
		m_canSlerp = false;
		m_targetCameraRotY = InnoMath::getQuatRotator(
			Vec4(0.0f, 1.0f, 0.0f, 0.0f),
			((-offset * m_rotateSpeed) / 180.0f) * PI<float>
		);
		m_playerTransformComponent->m_localTransformVector_target.m_rot = m_targetCameraRotY.quatMul(m_playerTransformComponent->m_localTransformVector_target.m_rot);
		m_playerCameraTransformComponent->m_localTransformVector_target.m_rot = m_targetCameraRotY.quatMul(m_playerCameraTransformComponent->m_localTransformVector_target.m_rot);

		m_canSlerp = true;
	}
}

void PlayerComponentCollection::rotateAroundRightAxis(float offset)
{
	if (m_canMove)
	{
		m_canSlerp = false;

		auto l_right = InnoMath::getDirection(Direction::Right, m_playerCameraTransformComponent->m_localTransformVector_target.m_rot);
		m_targetCameraRotX = InnoMath::getQuatRotator(
			l_right,
			((offset * m_rotateSpeed) / 180.0f) * PI<float>
		);
		m_playerCameraTransformComponent->m_localTransformVector_target.m_rot = m_targetCameraRotX.quatMul(m_playerCameraTransformComponent->m_localTransformVector_target.m_rot);

		m_canSlerp = true;
	}
}

namespace GameClientNS
{
	float seed = 0.0f;
	bool allowUpdate = true;

	std::vector<InnoEntity*> m_referenceSphereEntites;
	std::vector<TransformComponent*> m_referenceSphereTransformComponents;
	std::vector<VisibleComponent*> m_referenceSphereVisibleComponents;

	std::vector<InnoEntity*> m_opaqueSphereEntites;
	std::vector<TransformComponent*> m_opaqueSphereTransformComponents;
	std::vector<VisibleComponent*> m_opaqueSphereVisibleComponents;

	std::vector<InnoEntity*> m_transparentCubeEntites;
	std::vector<TransformComponent*> m_transparentCubeTransformComponents;
	std::vector<VisibleComponent*> m_transparentCubeVisibleComponents;

	std::vector<InnoEntity*> m_volumetricCubeEntites;
	std::vector<TransformComponent*> m_volumetricCubeTransformComponents;
	std::vector<VisibleComponent*> m_volumetricCubeVisibleComponents;

	std::vector<InnoEntity*> m_occlusionCubeEntites;
	std::vector<TransformComponent*> m_occlusionCubeTransformComponents;
	std::vector<VisibleComponent*> m_occlusionCubeVisibleComponents;

	std::vector<InnoEntity*> m_pointLightEntites;
	std::vector<TransformComponent*> m_pointLightTransformComponents;
	std::vector<LightComponent*> m_pointLightComponents;

	Vec4 m_posOffset;
	std::default_random_engine m_generator;

	bool setup();

	bool setupReferenceSpheres();
	bool setupOcclusionCubes();
	bool setupOpaqueSpheres();
	bool setupTransparentCubes();
	bool setupVolumetricCubes();
	bool setupPointLights();

	bool initialize();

	bool update();
	bool updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel = ShaderModel::Opaque);
	void updateSpheres();

	void runTest(uint32_t testTime, std::function<bool()> testCase);

	Vec4 getMousePositionInWorldSpace();

	std::function<void()> f_sceneLoadingFinishCallback;
	std::function<void()> f_loadTestScene;
	std::function<void()> f_convertModel;

	std::function<void()> f_runRayTracing;
	std::function<void()> f_pauseGame;

	ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
}

bool GameClientNS::setupReferenceSpheres()
{
	uint32_t l_matrixDim = 8;
	float l_breadthInterval = 4.0f;
	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_referenceSphereTransformComponents.clear();
	m_referenceSphereVisibleComponents.clear();
	m_referenceSphereEntites.clear();

	m_referenceSphereTransformComponents.reserve(l_containerSize);
	m_referenceSphereVisibleComponents.reserve(l_containerSize);
	m_referenceSphereEntites.reserve(l_containerSize);

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_referenceSphereTransformComponents.emplace_back();
		m_referenceSphereVisibleComponents.emplace_back();
		auto l_entityName = std::string("MaterialReferenceSphere_" + std::to_string(i) + "/");
		m_referenceSphereEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_referenceSphereTransformComponents[i] = SpawnComponent(TransformComponent, m_referenceSphereEntites[i], false, ObjectLifespan::Scene);
		m_referenceSphereTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_referenceSphereTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_referenceSphereVisibleComponents[i] = SpawnComponent(VisibleComponent, m_referenceSphereEntites[i], false, ObjectLifespan::Scene);
		m_referenceSphereVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Sphere;
		m_referenceSphereVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
		m_referenceSphereVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
		m_referenceSphereVisibleComponents[i]->m_simulatePhysics = true;
	}

	for (uint32_t i = 0; i < l_matrixDim; i++)
	{
		for (uint32_t j = 0; j < l_matrixDim; j++)
		{
			m_referenceSphereTransformComponents[i * l_matrixDim + j]->m_localTransformVector.m_pos =
				m_posOffset +
				Vec4(
					(-(l_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval) + 100.0f,
					2.0f,
					(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
					0.0f);
		}
	}

	return true;
}

bool GameClientNS::setupOcclusionCubes()
{
	uint32_t l_matrixDim = 8;
	float l_breadthInterval = 42.0f;
	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_occlusionCubeTransformComponents.clear();
	m_occlusionCubeVisibleComponents.clear();
	m_occlusionCubeEntites.clear();

	m_occlusionCubeTransformComponents.reserve(l_containerSize);
	m_occlusionCubeVisibleComponents.reserve(l_containerSize);
	m_occlusionCubeEntites.reserve(l_containerSize);

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_occlusionCubeTransformComponents.emplace_back();
		m_occlusionCubeVisibleComponents.emplace_back();
		auto l_entityName = std::string("OcclusionCube_" + std::to_string(i) + "/");
		m_occlusionCubeEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_occlusionCubeTransformComponents[i] = SpawnComponent(TransformComponent, m_occlusionCubeEntites[i], false, ObjectLifespan::Scene);
		m_occlusionCubeTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_occlusionCubeVisibleComponents[i] = SpawnComponent(VisibleComponent, m_occlusionCubeEntites[i], false, ObjectLifespan::Scene);
		m_occlusionCubeVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Cube;
		m_occlusionCubeVisibleComponents[i]->m_meshUsage = MeshUsage::Static;
		m_occlusionCubeVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
		m_occlusionCubeVisibleComponents[i]->m_simulatePhysics = true;
	}

	std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);
	std::uniform_real_distribution<float> l_randomHeightDelta(16.0f, 48.0f);
	std::uniform_real_distribution<float> l_randomWidthDelta(4.0f, 6.0f);
	std::uniform_real_distribution<float> l_randomDepthDelta(6.0f, 8.0f);

	auto l_halfMatrixDim = float(l_matrixDim - 1) / 2.0f;
	auto l_offset = l_halfMatrixDim * l_breadthInterval;

	for (uint32_t i = 0; i < l_matrixDim; i++)
	{
		for (uint32_t j = 0; j < l_matrixDim; j++)
		{
			auto l_currentComponent = m_occlusionCubeTransformComponents[i * l_matrixDim + j];

			auto l_heightOffset = l_halfMatrixDim * 3.0f - std::abs((float)i - l_halfMatrixDim) - std::abs((float)j - l_halfMatrixDim);
			l_heightOffset *= 4.0f;
			l_currentComponent->m_localTransformVector.m_scale =
				Vec4(l_randomWidthDelta(m_generator), l_heightOffset, l_randomDepthDelta(m_generator), 1.0f);

			l_currentComponent->m_localTransformVector.m_pos =
				m_posOffset +
				Vec4(
					(i * l_breadthInterval) - l_offset,
					l_currentComponent->m_localTransformVector.m_scale.y / 2.0f,
					(j * l_breadthInterval) - l_offset,
					0.0f);

			l_currentComponent->m_localTransformVector.m_rot =
				InnoMath::calcRotatedLocalRotator(l_currentComponent->m_localTransformVector.m_rot,
					Vec4(0.0f, 1.0f, 0.0f, 0.0f),
					l_randomRotDelta(m_generator));
		}
	}

	return true;
}

bool GameClientNS::setupOpaqueSpheres()
{
	uint32_t l_matrixDim = 8;
	float l_breadthInterval = 4.0f;
	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_opaqueSphereTransformComponents.clear();
	m_opaqueSphereVisibleComponents.clear();
	m_opaqueSphereEntites.clear();

	m_opaqueSphereTransformComponents.reserve(l_containerSize);
	m_opaqueSphereVisibleComponents.reserve(l_containerSize);
	m_opaqueSphereEntites.reserve(l_containerSize);

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_opaqueSphereTransformComponents.emplace_back();
		m_opaqueSphereVisibleComponents.emplace_back();
		auto l_entityName = std::string("PhysicsTestOpaqueObject_" + std::to_string(i) + "/");
		m_opaqueSphereEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_opaqueSphereTransformComponents[i] = SpawnComponent(TransformComponent, m_opaqueSphereEntites[i], false, ObjectLifespan::Scene);
		m_opaqueSphereTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_opaqueSphereTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_opaqueSphereVisibleComponents[i] = SpawnComponent(VisibleComponent, m_opaqueSphereEntites[i], false, ObjectLifespan::Scene);
		m_opaqueSphereVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape(i % 6 + 5);
		m_opaqueSphereVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
		m_opaqueSphereVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
		m_opaqueSphereVisibleComponents[i]->m_simulatePhysics = true;
	}

	std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
	std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);

	for (uint32_t i = 0; i < l_matrixDim; i++)
	{
		for (uint32_t j = 0; j < l_matrixDim; j++)
		{
			auto l_currentComponent = m_opaqueSphereTransformComponents[i * l_matrixDim + j];
			l_currentComponent->m_localTransformVector.m_pos =
				m_posOffset +
				Vec4(
					(-(l_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval),
					l_randomPosDelta(m_generator) * 50.0f,
					(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
					0.0f);

			l_currentComponent->m_localTransformVector.m_rot =
				InnoMath::calcRotatedLocalRotator(l_currentComponent->m_localTransformVector.m_rot,
					Vec4(l_randomPosDelta(m_generator), l_randomPosDelta(m_generator), l_randomPosDelta(m_generator), 0.0f).normalize(),
					l_randomRotDelta(m_generator));
		}
	}

	return true;
}

bool GameClientNS::setupTransparentCubes()
{
	float l_breadthInterval = 4.0f;
	uint32_t l_containerSize = 8;

	m_transparentCubeTransformComponents.clear();
	m_transparentCubeVisibleComponents.clear();
	m_transparentCubeEntites.clear();

	m_transparentCubeTransformComponents.reserve(l_containerSize);
	m_transparentCubeVisibleComponents.reserve(l_containerSize);
	m_transparentCubeEntites.reserve(l_containerSize);

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_transparentCubeTransformComponents.emplace_back();
		m_transparentCubeVisibleComponents.emplace_back();
		auto l_entityName = std::string("PhysicsTestTransparentCube_" + std::to_string(i) + "/");
		m_transparentCubeEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_transparentCubeTransformComponents[i] = SpawnComponent(TransformComponent, m_transparentCubeEntites[i], false, ObjectLifespan::Scene);
		m_transparentCubeTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_transparentCubeTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f * i, 1.0f * i, 0.5f, 1.0f);
		m_transparentCubeVisibleComponents[i] = SpawnComponent(VisibleComponent, m_transparentCubeEntites[i], false, ObjectLifespan::Scene);
		m_transparentCubeVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Cube;
		m_transparentCubeVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
		m_transparentCubeVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
		m_transparentCubeVisibleComponents[i]->m_simulatePhysics = true;
	}

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_transparentCubeTransformComponents[i]->m_localTransformVector.m_pos = Vec4(0.0f, 2.0f * i, -(i * l_breadthInterval) - 4.0f, 1.0f);
	}

	return true;
}

bool GameClientNS::setupVolumetricCubes()
{
	float l_breadthInterval = 4.0f;
	uint32_t l_containerSize = 8;

	m_volumetricCubeTransformComponents.clear();
	m_volumetricCubeVisibleComponents.clear();
	m_volumetricCubeEntites.clear();

	m_volumetricCubeTransformComponents.reserve(l_containerSize);
	m_volumetricCubeVisibleComponents.reserve(l_containerSize);
	m_volumetricCubeEntites.reserve(l_containerSize);

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_volumetricCubeTransformComponents.emplace_back();
		m_volumetricCubeVisibleComponents.emplace_back();
		auto l_entityName = std::string("PhysicsTestVolumetricCube_" + std::to_string(i) + "/");
		m_volumetricCubeEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_volumetricCubeTransformComponents[i] = SpawnComponent(TransformComponent, m_volumetricCubeEntites[i], false, ObjectLifespan::Scene);
		m_volumetricCubeTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_volumetricCubeTransformComponents[i]->m_localTransformVector.m_scale = Vec4(4.0f, 4.0f, 4.0f, 1.0f);
		m_volumetricCubeVisibleComponents[i] = SpawnComponent(VisibleComponent, m_volumetricCubeEntites[i], false, ObjectLifespan::Scene);
		m_volumetricCubeVisibleComponents[i]->m_proceduralMeshShape = ProceduralMeshShape::Cube;
		m_volumetricCubeVisibleComponents[i]->m_meshUsage = MeshUsage::Dynamic;
		m_volumetricCubeVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TriangleStrip;
		m_volumetricCubeVisibleComponents[i]->m_simulatePhysics = false;
	}

	std::uniform_real_distribution<float> l_randomPosDelta(-40.0f, 40.0f);

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_volumetricCubeTransformComponents[i]->m_localTransformVector.m_pos = Vec4(l_randomPosDelta(m_generator), 2.0f, l_randomPosDelta(m_generator), 1.0f);
	}

	return true;
}

bool GameClientNS::setupPointLights()
{
	uint32_t l_matrixDim = 16;
	float l_breadthInterval = 4.0f;

	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_pointLightTransformComponents.clear();
	m_pointLightComponents.clear();
	m_pointLightEntites.clear();

	m_pointLightTransformComponents.reserve(l_containerSize);
	m_pointLightComponents.reserve(l_containerSize);
	m_pointLightEntites.reserve(l_containerSize);

	std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
	std::uniform_real_distribution<float> l_randomLuminousFlux(10.0f, 100.0f);
	std::uniform_real_distribution<float> l_randomColorTemperature(2000.0f, 14000.0f);

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_pointLightTransformComponents.emplace_back();
		m_pointLightComponents.emplace_back();
		auto l_entityName = std::string("TestPointLight_" + std::to_string(i) + "/");
		m_pointLightEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(false, ObjectLifespan::Scene, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (uint32_t i = 0; i < l_containerSize; i++)
	{
		m_pointLightTransformComponents[i] = SpawnComponent(TransformComponent, m_pointLightEntites[i], false, ObjectLifespan::Scene);
		m_pointLightTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_pointLightTransformComponents[i]->m_localTransformVector.m_scale = Vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_pointLightComponents[i] = SpawnComponent(LightComponent, m_pointLightEntites[i], false, ObjectLifespan::Scene);
		m_pointLightComponents[i]->m_LightType = LightType::Point;
		m_pointLightComponents[i]->m_LuminousFlux = l_randomLuminousFlux(m_generator);
		m_pointLightComponents[i]->m_ColorTemperature = l_randomColorTemperature(m_generator);
	}

	for (uint32_t i = 0; i < l_matrixDim; i++)
	{
		for (uint32_t j = 0; j < l_matrixDim; j++)
		{
			m_pointLightTransformComponents[i * l_matrixDim + j]->m_localTransformVector.m_pos =
				m_playerCameraTransformComponent->m_localTransformVector.m_pos +
				Vec4(
					(-(l_matrixDim - 1.0f) * l_breadthInterval * l_randomPosDelta(m_generator) / 2.0f)
					+ (i * l_breadthInterval), l_randomPosDelta(m_generator) * 32.0f,
					(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
					0.0f);
		}
	}

	return true;
}

bool GameClientNS::setup()
{
	auto l_testQuatToMat = [&]() -> bool {
		std::uniform_real_distribution<float> randomAxis(0.0f, 1.0f);
		auto axisSample = Vec4(randomAxis(m_generator) * 2.0f - 1.0f, randomAxis(m_generator) * 2.0f - 1.0f, randomAxis(m_generator) * 2.0f - 1.0f, 0.0f);
		axisSample = axisSample.normalize();

		std::uniform_real_distribution<float> randomAngle(0.0f, 360.0f);
		auto angleSample = randomAngle(m_generator);

		Vec4 originalRot = InnoMath::getQuatRotator(axisSample, angleSample);
		Mat4 rotMat = InnoMath::toRotationMatrix(originalRot);
		auto resultRot = InnoMath::toQuatRotator(rotMat);

		auto testResult = true;
		testResult &= (std::abs(std::abs(originalRot.w) - std::abs(resultRot.w)) < epsilon<float, 4>);
		testResult &= (std::abs(std::abs(originalRot.x) - std::abs(resultRot.x)) < epsilon<float, 4>);
		testResult &= (std::abs(std::abs(originalRot.y) - std::abs(resultRot.y)) < epsilon<float, 4>);
		testResult &= (std::abs(std::abs(originalRot.z) - std::abs(resultRot.z)) < epsilon<float, 4>);

		return testResult;
	};

	runTest(512, l_testQuatToMat);

	f_runRayTracing = [&]() { g_pModuleManager->getRenderingFrontend()->runRayTrace(); };
	f_pauseGame = [&]() { allowUpdate = !allowUpdate; };

	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_N, true }, ButtonEvent{ EventLifeTime::OneShot, &f_runRayTracing });
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_F, true }, ButtonEvent{ EventLifeTime::OneShot, &f_pauseGame });

	f_sceneLoadingFinishCallback = [&]() {
		m_posOffset = m_playerCameraTransformComponent->m_localTransformVector.m_pos;
		m_posOffset.z -= 75.0f;

		m_posOffset = Vec4(0.0f, 0.0f, 0.0f, 1.0f);

		setupReferenceSpheres();
		//setupOcclusionCubes();
		setupOpaqueSpheres();
		setupTransparentCubes();
		setupVolumetricCubes();
		//setupPointLights();

		m_ObjectStatus = ObjectStatus::Activated;
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback, 0);

	return true;
}

bool GameClientNS::initialize()
{
	f_loadTestScene = []()
	{
		//g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestBox.InnoScene");
		//g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestSibenik.InnoScene");
		//g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestSponza.InnoScene");
		g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestFireplaceRoom.InnoScene");
	};

	f_convertModel = []()
	{
		g_pModuleManager->getAssetSystem()->convertModel("..//Res//Models//Wolf//Wolf.fbx", "..//Res//ConvertedAssets//");
	};
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_R, true }, ButtonEvent{ EventLifeTime::OneShot, &f_loadTestScene });
	g_pModuleManager->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_Y, true }, ButtonEvent{ EventLifeTime::OneShot, &f_convertModel });

	return true;
}

bool GameClientNS::updateMaterial(Model* model, Vec4 albedo, Vec4 MRAT, ShaderModel shaderModel)
{
	if (model)
	{
		for (uint64_t j = 0; j < model->meshMaterialPairs.m_count; j++)
		{
			auto l_pair = g_pModuleManager->getAssetSystem()->getMeshMaterialPair(model->meshMaterialPairs.m_startOffset + j);
			l_pair->material->m_materialAttributes.AlbedoR = albedo.x;
			l_pair->material->m_materialAttributes.AlbedoG = albedo.y;
			l_pair->material->m_materialAttributes.AlbedoB = albedo.z;
			l_pair->material->m_materialAttributes.Metallic = MRAT.x;
			l_pair->material->m_materialAttributes.Roughness = MRAT.y;
			l_pair->material->m_materialAttributes.AO = MRAT.z;
			l_pair->material->m_materialAttributes.Alpha = albedo.w;
			l_pair->material->m_materialAttributes.Thickness = MRAT.w;
			l_pair->material->m_ShaderModel = shaderModel;
		}
	}
	return true;
};

bool DefaultLogicClient::setup()
{
	bool l_result = true;
	l_result = l_result && PlayerComponentCollection::setup();
	l_result = l_result && GameClientNS::setup();

	return l_result;
}

bool DefaultLogicClient::initialize()
{
	bool l_result = true;
	g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//default.InnoScene");

	//g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestBox.InnoScene");
	//g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestSibenik.InnoScene");
	//g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestSponza.InnoScene");
	//g_pModuleManager->getFileSystem()->loadScene("..//Res//Scenes//GITestFireplaceRoom.InnoScene");

	l_result = l_result && PlayerComponentCollection::initialize();
	l_result = l_result && GameClientNS::initialize();

	return l_result;
}

bool DefaultLogicClient::update()
{
	return GameClientNS::update();
}

bool DefaultLogicClient::terminate()
{
	GameClientNS::m_ObjectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus DefaultLogicClient::getStatus()
{
	return GameClientNS::m_ObjectStatus;
}

std::string DefaultLogicClient::getApplicationName()
{
	return std::string("InnoGameClient/");
}

bool GameClientNS::update()
{
	if (m_ObjectStatus == ObjectStatus::Activated)
	{
		if (allowUpdate)
		{
			auto l_tickTime = g_pModuleManager->getTickTime();
			seed += (l_tickTime / 1000.0f);

			auto l_seed = (1.0f - l_tickTime / 100.0f);
			l_seed = l_seed > 0.0f ? l_seed : 0.01f;
			l_seed = l_seed > 0.85f ? 0.85f : l_seed;

			PlayerComponentCollection::update(l_seed);

			if (AnimationStateMachine::m_ObjectStatus == ObjectStatus::Activated)
			{
				AnimationStateMachine::simulate();
			}

			updateSpheres();
		}
	}

	return true;
}

void GameClientNS::runTest(uint32_t testTime, std::function<bool()> testCase)
{
	g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "Start test...");
	for (uint32_t i = 0; i < testTime; i++)
	{
		auto l_result = testCase();
		if (!l_result)
		{
			g_pModuleManager->getLogSystem()->Log(LogLevel::Warning, "Test failure.");
		}
	}
	g_pModuleManager->getLogSystem()->Log(LogLevel::Verbose, "Finished test for ", testTime, " times.");
}

Vec4 GameClientNS::getMousePositionInWorldSpace()
{
	auto l_screenResolution = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_mousePositionSS = g_pModuleManager->getEventSystem()->getMousePosition();

	auto l_x = 2.0f * l_mousePositionSS.x / l_screenResolution.x - 1.0f;
	auto l_y = 1.0f - 2.0f * l_mousePositionSS.y / l_screenResolution.y;
	auto l_z = -1.0f;
	auto l_w = 1.0f;
	Vec4 l_ndcSpace = Vec4(l_x, l_y, l_z, l_w);

	auto l_mainCamera = GetComponentManager(CameraComponent)->Get(0);
	if (l_mainCamera == nullptr)
	{
		return Vec4();
	}
	auto l_cameraTransformComponent = GetComponent(TransformComponent, l_mainCamera->m_Owner);
	if (l_cameraTransformComponent == nullptr)
	{
		return Vec4();
	}
	auto pCamera = l_mainCamera->m_projectionMatrix;
	auto rCamera =
		InnoMath::getInvertRotationMatrix(
			l_cameraTransformComponent->m_globalTransformVector.m_rot
		);
	auto tCamera =
		InnoMath::getInvertTranslationMatrix(
			l_cameraTransformComponent->m_globalTransformVector.m_pos
		);
	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	l_ndcSpace = InnoMath::mul(l_ndcSpace, pCamera.inverse());
	l_ndcSpace.z = -1.0f;
	l_ndcSpace.w = 0.0f;
	l_ndcSpace = InnoMath::mul(l_ndcSpace, rCamera.inverse());
	l_ndcSpace = InnoMath::mul(l_ndcSpace, tCamera.inverse());
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT

	l_ndcSpace = InnoMath::mul(pCamera.inverse(), l_ndcSpace);
	l_ndcSpace.z = -1.0f;
	l_ndcSpace.w = 0.0f;
	l_ndcSpace = InnoMath::mul(tCamera.inverse(), l_ndcSpace);
	l_ndcSpace = InnoMath::mul(rCamera.inverse(), l_ndcSpace);
#endif
	l_ndcSpace = l_ndcSpace.normalize();
	return l_ndcSpace;
}

void PlayerComponentCollection::update(float seed)
{
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
}

void GameClientNS::updateSpheres()
{
	for (uint32_t i = 0; i < m_opaqueSphereVisibleComponents.size(); i += 4)
	{
		auto l_albedoFactor1 = (sin(seed / 2.0f + i) + 1.0f) / 2.0f;
		auto l_albedoFactor2 = (sin(seed / 3.0f + i) + 1.0f) / 2.0f;
		auto l_albedoFactor3 = (sin(seed / 5.0f + i) + 1.0f) / 2.0f;

		auto l_albedo1 = Vec4(l_albedoFactor1, l_albedoFactor2, l_albedoFactor3, 1.0f);
		auto l_albedo2 = Vec4(l_albedoFactor3, l_albedoFactor2, l_albedoFactor1, 1.0f);
		auto l_albedo3 = Vec4(l_albedoFactor2, l_albedoFactor3, l_albedoFactor1, 1.0f);
		auto l_albedo4 = Vec4(l_albedoFactor2, l_albedoFactor1, l_albedoFactor3, 1.0f);

		auto l_MRATFactor1 = ((sin(seed / 4.0f + i) + 1.0f) / 2.001f);
		auto l_MRATFactor2 = ((sin(seed / 5.0f + i) + 1.0f) / 2.001f);
		auto l_MRATFactor3 = ((sin(seed / 6.0f + i) + 1.0f) / 2.001f);

		updateMaterial(m_opaqueSphereVisibleComponents[i]->m_model, l_albedo1, Vec4(l_MRATFactor1, l_MRATFactor2, 0.0f, 0.0f));
		updateMaterial(m_opaqueSphereVisibleComponents[i + 1]->m_model, l_albedo2, Vec4(l_MRATFactor2, l_MRATFactor1, 0.0f, 0.0f));
		updateMaterial(m_opaqueSphereVisibleComponents[i + 2]->m_model, l_albedo3, Vec4(l_MRATFactor3, l_MRATFactor2, 0.0f, 0.0f));
		updateMaterial(m_opaqueSphereVisibleComponents[i + 3]->m_model, l_albedo4, Vec4(l_MRATFactor3, l_MRATFactor1, 0.0f, 0.0f));
	}

	for (uint32_t i = 0; i < m_transparentCubeVisibleComponents.size(); i++)
	{
		auto l_albedo = InnoMath::HSVtoRGB(Vec4((sin(seed / 6.0f + i) * 0.5f + 0.5f) * 360.0f, 1.0f, 1.0f, 0.5f));
		l_albedo.w = sin(seed / 6.0f + i) * 0.5f + 0.5f;
		auto l_MRAT = Vec4(0.0f, sin(seed / 4.0f + i) * 0.5f + 0.5f, 1.0f, clamp((float)sin(seed / 5.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f));
		updateMaterial(m_transparentCubeVisibleComponents[i]->m_model, l_albedo, l_MRAT, ShaderModel::Transparent);
	}

	for (uint32_t i = 0; i < m_volumetricCubeVisibleComponents.size(); i++)
	{
		auto l_albedo = InnoMath::HSVtoRGB(Vec4((sin(seed / 6.0f + i) * 0.5f + 0.5f) * 360.0f, 1.0f, 1.0f, 0.5f));
		l_albedo.w = clamp((float)sin(seed / 7.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f);
		auto l_MRAT = Vec4(clamp((float)sin(seed / 5.0f + i) * 0.5f + 0.5f, epsilon<float, 4>, 1.0f), 1.0f, 1.0f, 1.0f);
		updateMaterial(m_volumetricCubeVisibleComponents[i]->m_model, l_albedo, l_MRAT, ShaderModel::Volumetric);
	}

	uint32_t l_matrixDim = 8;
	for (uint32_t i = 0; i < l_matrixDim; i++)
	{
		for (uint32_t j = 0; j < l_matrixDim; j++)
		{
			auto l_MRAT = Vec4((float)i / (float)(l_matrixDim - 1), (float)j / (float)(l_matrixDim - 1), 0.0f, 1.0f);
			updateMaterial(m_referenceSphereVisibleComponents[i * l_matrixDim + j]->m_model, Vec4(1.0f, 1.0f, 1.0f, 1.0f), l_MRAT);
		}
	}
}