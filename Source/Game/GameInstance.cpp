#include "GameInstance.h"
#include "../../Engine/Common/CommonMacro.inl"
#include "../../Engine/ComponentManager/ITransformComponentManager.h"
#include "../../Engine/ComponentManager/IVisibleComponentManager.h"
#include "../../Engine/ComponentManager/IPointLightComponentManager.h"
#include "../../Engine/ComponentManager/ISphereLightComponentManager.h"
#include "../../Engine/ComponentManager/ICameraComponentManager.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

namespace PlayerComponentCollection
{
	bool setup();
	bool initialize();

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	InnoEntity* m_cameraParentEntity;

	TransformComponent* m_cameraTransformComponent;
	CameraComponent* m_cameraComponent;

	std::function<void()> f_moveForward;
	std::function<void()> f_moveBackward;
	std::function<void()> f_moveLeft;
	std::function<void()> f_moveRight;

	std::function<void()> f_allowMove;
	std::function<void()> f_forbidMove;

	std::function<void()> f_speedUp;
	std::function<void()> f_speedDown;

	std::function<void(float)> f_rotateAroundPositiveYAxis;
	std::function<void(float)> f_rotateAroundRightAxis;

	float m_initialMoveSpeed = 0;
	float m_moveSpeed = 0;
	float m_rotateSpeed = 0;
	bool m_canMove = false;
	bool m_canSlerp = false;
	bool m_smoothInterp = true;

	void move(vec4 direction, float length);
	vec4 m_targetPawnPos;
	vec4 m_targetCameraPos;
	vec4 m_targetCameraRot;
	vec4 m_targetCameraRotX;
	vec4 m_targetCameraRotY;

	void update(float seed);

	void rotateAroundPositiveYAxis(float offset);
	void rotateAroundRightAxis(float offset);

	std::function<void()> f_sceneLoadingFinishCallback;
};

bool PlayerComponentCollection::setup()
{
	f_sceneLoadingFinishCallback = [&]() {
		auto l_cameraEntity = g_pModuleManager->getEntityManager()->Find("playerCharacterCamera");
		if (l_cameraEntity.has_value())
		{
			m_cameraParentEntity = l_cameraEntity.value();
		}
		else
		{
			m_cameraParentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Gameplay, "playerCharacterCamera/");
		}
		m_cameraTransformComponent = GetComponent(TransformComponent, m_cameraParentEntity);
		m_cameraComponent = GetComponent(CameraComponent, m_cameraParentEntity);

		m_targetCameraPos = m_cameraTransformComponent->m_localTransformVector.m_pos;
		m_targetCameraRot = m_cameraTransformComponent->m_localTransformVector.m_rot;
		m_targetCameraRotX = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		m_targetCameraRotY = vec4(0.0f, 0.0f, 0.0f, 1.0f);

		f_moveForward = [&]() { move(InnoMath::getDirection(direction::FORWARD, m_cameraTransformComponent->m_localTransformVector.m_rot), m_moveSpeed); };
		f_moveBackward = [&]() { move(InnoMath::getDirection(direction::BACKWARD, m_cameraTransformComponent->m_localTransformVector.m_rot), m_moveSpeed); };
		f_moveLeft = [&]() { move(InnoMath::getDirection(direction::LEFT, m_cameraTransformComponent->m_localTransformVector.m_rot), m_moveSpeed); };
		f_moveRight = [&]() { move(InnoMath::getDirection(direction::RIGHT, m_cameraTransformComponent->m_localTransformVector.m_rot), m_moveSpeed); };

		f_speedUp = [&]() { m_moveSpeed = m_initialMoveSpeed * 10.0f; };
		f_speedDown = [&]() { m_moveSpeed = m_initialMoveSpeed; };

		f_allowMove = [&]() { m_canMove = true; };
		f_forbidMove = [&]() { m_canMove = false; };

		f_rotateAroundPositiveYAxis = std::bind(&rotateAroundPositiveYAxis, std::placeholders::_1);
		f_rotateAroundRightAxis = std::bind(&rotateAroundRightAxis, std::placeholders::_1);

		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_S, ButtonStatus::PRESSED }, &f_moveForward);
		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_W, ButtonStatus::PRESSED }, &f_moveBackward);
		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_A, ButtonStatus::PRESSED }, &f_moveLeft);
		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_D, ButtonStatus::PRESSED }, &f_moveRight);

		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_SPACE, ButtonStatus::PRESSED }, &f_speedUp);
		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_SPACE, ButtonStatus::RELEASED }, &f_speedDown);

		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_RIGHT, ButtonStatus::PRESSED }, &f_allowMove);
		g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_MOUSE_BUTTON_RIGHT, ButtonStatus::RELEASED }, &f_forbidMove);
		g_pModuleManager->getEventSystem()->addMouseMovementCallback(0, &f_rotateAroundPositiveYAxis);
		g_pModuleManager->getEventSystem()->addMouseMovementCallback(1, &f_rotateAroundRightAxis);

		m_initialMoveSpeed = 0.5f;
		m_moveSpeed = m_initialMoveSpeed;
		m_rotateSpeed = 10.0f;
	};
	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	return true;
}

bool PlayerComponentCollection::initialize()
{
	return true;
}

void PlayerComponentCollection::move(vec4 direction, float length)
{
	if (m_canMove)
	{
		auto l_currentCameraPos = m_cameraTransformComponent->m_localTransformVector.m_pos;
		m_targetCameraPos = InnoMath::moveTo(l_currentCameraPos, direction, length);
	}
}

void PlayerComponentCollection::rotateAroundPositiveYAxis(float offset)
{
	if (m_canMove)
	{
		m_canSlerp = false;

		m_targetCameraRotY = InnoMath::getQuatRotator(
			vec4(0.0f, 1.0f, 0.0f, 0.0f),
			((-offset * m_rotateSpeed) / 180.0f)* PI<float>
		);
		m_targetCameraRot = m_targetCameraRotY.quatMul(m_targetCameraRot);

		m_canSlerp = true;
	}
}

void PlayerComponentCollection::rotateAroundRightAxis(float offset)
{
	if (m_canMove)
	{
		m_canSlerp = false;

		auto l_right = InnoMath::getDirection(direction::RIGHT, m_targetCameraRot);
		m_targetCameraRotX = InnoMath::getQuatRotator(
			l_right,
			((offset * m_rotateSpeed) / 180.0f)* PI<float>
		);
		m_targetCameraRot = m_targetCameraRotX.quatMul(m_targetCameraRot);

		m_canSlerp = true;
	}
}

namespace GameInstanceNS
{
	float seed = 0.0f;

	std::vector<InnoEntity*> m_referenceSphereEntites;
	std::vector<TransformComponent*> m_referenceSphereTransformComponents;
	std::vector<VisibleComponent*> m_referenceSphereVisibleComponents;

	std::vector<InnoEntity*> m_opaqueSphereEntites;
	std::vector<TransformComponent*> m_opaqueSphereTransformComponents;
	std::vector<VisibleComponent*> m_opaqueSphereVisibleComponents;

	std::vector<InnoEntity*> m_transparentSphereEntites;
	std::vector<TransformComponent*> m_transparentSphereTransformComponents;
	std::vector<VisibleComponent*> m_transparentSphereVisibleComponents;

	std::vector<InnoEntity*> m_pointLightEntites;
	std::vector<TransformComponent*> m_pointLightTransformComponents;
	std::vector<PointLightComponent*> m_pointLightComponents;

	bool setup();

	bool setupReferenceSpheres();
	bool setupOpaqueSpheres();
	bool setupTransparentSpheres();
	bool setupPointLights();

	bool initialize();

	bool update();
	bool updateMaterial(const ModelMap& modelMap, vec4 albedo, vec4 MRAT);
	void updateSpheres();

	void runTest(unsigned int testTime, std::function<bool()> testCase);

	std::function<void()> f_sceneLoadingFinishCallback;
	std::function<void()> f_testFunc;

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
}

bool GameInstanceNS::setupReferenceSpheres()
{
	unsigned int l_matrixDim = 8;
	float l_breadthInterval = 4.0f;
	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_referenceSphereTransformComponents.clear();
	m_referenceSphereVisibleComponents.clear();
	m_referenceSphereEntites.clear();

	m_referenceSphereTransformComponents.reserve(l_containerSize);
	m_referenceSphereVisibleComponents.reserve(l_containerSize);
	m_referenceSphereEntites.reserve(l_containerSize);

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_referenceSphereTransformComponents.emplace_back();
		m_referenceSphereVisibleComponents.emplace_back();
		auto l_entityName = std::string("MaterialReferenceSphere_" + std::to_string(i) + "/");
		m_referenceSphereEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Gameplay, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_referenceSphereTransformComponents[i] = SpawnComponent(TransformComponent, m_referenceSphereEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		m_referenceSphereTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_referenceSphereTransformComponents[i]->m_localTransformVector.m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_referenceSphereVisibleComponents[i] = SpawnComponent(VisibleComponent, m_referenceSphereEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		m_referenceSphereVisibleComponents[i]->m_visiblilityType = VisiblilityType::INNO_OPAQUE;
		m_referenceSphereVisibleComponents[i]->m_meshShapeType = MeshShapeType::SPHERE;
		m_referenceSphereVisibleComponents[i]->m_meshUsageType = MeshUsageType::DYNAMIC;
		m_referenceSphereVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
		m_referenceSphereVisibleComponents[i]->m_simulatePhysics = true;
	}

	for (unsigned int i = 0; i < l_matrixDim; i++)
	{
		for (unsigned int j = 0; j < l_matrixDim; j++)
		{
			m_referenceSphereTransformComponents[i * l_matrixDim + j]->m_localTransformVector.m_pos =
				vec4(
				(-(l_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval) + 100.0f,
					2.0f,
					(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
					1.0f);
		}
	}

	return true;
}

bool GameInstanceNS::setupOpaqueSpheres()
{
	unsigned int l_matrixDim = 8;
	float l_breadthInterval = 4.0f;
	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_opaqueSphereTransformComponents.clear();
	m_opaqueSphereVisibleComponents.clear();
	m_opaqueSphereEntites.clear();

	m_opaqueSphereTransformComponents.reserve(l_containerSize);
	m_opaqueSphereVisibleComponents.reserve(l_containerSize);
	m_opaqueSphereEntites.reserve(l_containerSize);

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_opaqueSphereTransformComponents.emplace_back();
		m_opaqueSphereVisibleComponents.emplace_back();
		auto l_entityName = std::string("PhysicsTestOpaqueObject_" + std::to_string(i) + "/");
		m_opaqueSphereEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Gameplay, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_opaqueSphereTransformComponents[i] = SpawnComponent(TransformComponent, m_opaqueSphereEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		m_opaqueSphereTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_opaqueSphereTransformComponents[i]->m_localTransformVector.m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_opaqueSphereVisibleComponents[i] = SpawnComponent(VisibleComponent, m_opaqueSphereEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		m_opaqueSphereVisibleComponents[i]->m_visiblilityType = VisiblilityType::INNO_OPAQUE;
		m_opaqueSphereVisibleComponents[i]->m_meshShapeType = (i & 0x00000001) ? MeshShapeType::SPHERE : MeshShapeType::CUBE;
		m_opaqueSphereVisibleComponents[i]->m_meshUsageType = MeshUsageType::DYNAMIC;
		m_opaqueSphereVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
		m_opaqueSphereVisibleComponents[i]->m_simulatePhysics = true;
	}

	std::default_random_engine l_generator;
	std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);
	std::uniform_real_distribution<float> l_randomRotDelta(0.0f, 180.0f);

	for (unsigned int i = 0; i < l_matrixDim; i++)
	{
		for (unsigned int j = 0; j < l_matrixDim; j++)
		{
			auto l_currentComponent = m_opaqueSphereTransformComponents[i * l_matrixDim + j];
			l_currentComponent->m_localTransformVector.m_pos =
				vec4(
				(-(l_matrixDim - 1.0f) * l_breadthInterval / 2.0f) + (i * l_breadthInterval),
					l_randomPosDelta(l_generator) * 50.0f,
					(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
					1.0f);

			l_currentComponent->m_localTransformVector.m_rot =
				InnoMath::caclRotatedLocalRotator(l_currentComponent->m_localTransformVector.m_rot,
					vec4(l_randomPosDelta(l_generator), l_randomPosDelta(l_generator), l_randomPosDelta(l_generator), 0.0f).normalize(),
					l_randomRotDelta(l_generator));
		}
	}

	return true;
}

bool GameInstanceNS::setupTransparentSpheres()
{
	unsigned int l_matrixDim = 8;
	float l_breadthInterval = 4.0f;
	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_transparentSphereTransformComponents.clear();
	m_transparentSphereVisibleComponents.clear();
	m_transparentSphereEntites.clear();

	m_transparentSphereTransformComponents.reserve(l_containerSize);
	m_transparentSphereVisibleComponents.reserve(l_containerSize);
	m_transparentSphereEntites.reserve(l_containerSize);

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_transparentSphereTransformComponents.emplace_back();
		m_transparentSphereVisibleComponents.emplace_back();
		auto l_entityName = std::string("PhysicsTestTransparentSphere_" + std::to_string(i) + "/");
		m_transparentSphereEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Gameplay, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_transparentSphereTransformComponents[i] = SpawnComponent(TransformComponent, m_transparentSphereEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		m_transparentSphereTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_transparentSphereTransformComponents[i]->m_localTransformVector.m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_transparentSphereVisibleComponents[i] = SpawnComponent(VisibleComponent, m_transparentSphereEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		m_transparentSphereVisibleComponents[i]->m_visiblilityType = VisiblilityType::INNO_TRANSPARENT;
		m_transparentSphereVisibleComponents[i]->m_meshShapeType = MeshShapeType::SPHERE;
		m_transparentSphereVisibleComponents[i]->m_meshUsageType = MeshUsageType::DYNAMIC;
		m_transparentSphereVisibleComponents[i]->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
		m_transparentSphereVisibleComponents[i]->m_simulatePhysics = true;
	}

	for (unsigned int i = 0; i < l_matrixDim; i++)
	{
		for (unsigned int j = 0; j < l_matrixDim; j++)
		{
			m_transparentSphereTransformComponents[i * l_matrixDim + j]->m_localTransformVector.m_pos =
				vec4(
				(-(l_matrixDim - 1.0f) * l_breadthInterval / 2.0f)
					+ (i * l_breadthInterval),
					5.0f,
					(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
					1.0f);
		}
	}

	return true;
}

bool GameInstanceNS::setupPointLights()
{
	unsigned int l_matrixDim = 16;
	float l_breadthInterval = 4.0f;

	auto l_containerSize = l_matrixDim * l_matrixDim;

	m_pointLightTransformComponents.clear();
	m_pointLightComponents.clear();
	m_pointLightEntites.clear();

	m_pointLightTransformComponents.reserve(l_containerSize);
	m_pointLightComponents.reserve(l_containerSize);
	m_pointLightEntites.reserve(l_containerSize);

	std::default_random_engine l_generator;
	std::uniform_real_distribution<float> l_randomPosDelta(0.0f, 1.0f);

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_pointLightTransformComponents.emplace_back();
		m_pointLightComponents.emplace_back();
		auto l_entityName = std::string("TestPointLight_" + std::to_string(i) + "/");
		m_pointLightEntites.emplace_back(g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Gameplay, l_entityName.c_str()));
	}

	auto l_rootTranformComponent = const_cast<TransformComponent*>(GetComponentManager(TransformComponent)->GetRootTransformComponent());

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_pointLightTransformComponents[i] = SpawnComponent(TransformComponent, m_pointLightEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		g_pModuleManager->getComponentManager(ComponentType::TransformComponent);
		m_pointLightTransformComponents[i]->m_parentTransformComponent = l_rootTranformComponent;
		m_pointLightTransformComponents[i]->m_localTransformVector.m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_pointLightComponents[i] = SpawnComponent(PointLightComponent, m_pointLightEntites[i], ObjectSource::Runtime, ObjectUsage::Gameplay);
		m_pointLightComponents[i]->m_luminousFlux = 100.0f;
		m_pointLightComponents[i]->m_color = vec4(l_randomPosDelta(l_generator), l_randomPosDelta(l_generator), l_randomPosDelta(l_generator), 1.0f);
	}

	for (unsigned int i = 0; i < l_matrixDim; i++)
	{
		for (unsigned int j = 0; j < l_matrixDim; j++)
		{
			m_pointLightTransformComponents[i * l_matrixDim + j]->m_localTransformVector.m_pos =
				vec4(
				(-(l_matrixDim - 1.0f) * l_breadthInterval * l_randomPosDelta(l_generator) / 2.0f)
					+ (i * l_breadthInterval), l_randomPosDelta(l_generator) * 32.0f,
					(j * l_breadthInterval) - 2.0f * (l_matrixDim - 1),
					1.0f);
		}
	}

	return true;
}

bool GameInstanceNS::setup()
{
	auto l_testQuatToMat = []() -> bool {
		std::default_random_engine generator;

		std::uniform_real_distribution<float> randomAxis(0.0f, 1.0f);
		auto axisSample = vec4(randomAxis(generator) * 2.0f - 1.0f, randomAxis(generator) * 2.0f - 1.0f, randomAxis(generator) * 2.0f - 1.0f, 0.0f);
		axisSample = axisSample.normalize();

		std::uniform_real_distribution<float> randomAngle(0.0f, 360.0f);
		auto angleSample = randomAngle(generator);

		vec4 originalRot = InnoMath::getQuatRotator(axisSample, angleSample);
		mat4 rotMat = InnoMath::toRotationMatrix(originalRot);
		auto resultRot = InnoMath::toQuatRotator(rotMat);

		auto testResult = true;
		testResult &= (std::abs(std::abs(originalRot.w) - std::abs(resultRot.w)) < epsilon4<float>);
		testResult &= (std::abs(std::abs(originalRot.x) - std::abs(resultRot.x)) < epsilon4<float>);
		testResult &= (std::abs(std::abs(originalRot.y) - std::abs(resultRot.y)) < epsilon4<float>);
		testResult &= (std::abs(std::abs(originalRot.z) - std::abs(resultRot.z)) < epsilon4<float>);

		return testResult;
	};

	runTest(512, l_testQuatToMat);

	f_sceneLoadingFinishCallback = [&]() {
		setupReferenceSpheres();
		setupOpaqueSpheres();
		setupTransparentSpheres();
		//setupPointLights();

		m_objectStatus = ObjectStatus::Activated;
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	return true;
}

bool GameInstanceNS::initialize()
{
	f_testFunc = []() {	g_pModuleManager->getFileSystem()->loadScene("Res//Scenes//animationTest.InnoScene");
	};
	g_pModuleManager->getEventSystem()->addButtonStatusCallback(ButtonData{ INNO_KEY_R, ButtonStatus::PRESSED }, &f_testFunc);
	return true;
}

bool GameInstanceNS::updateMaterial(const ModelMap& modelMap, vec4 albedo, vec4 MRAT)
{
	for (auto& j : modelMap)
	{
		j.second->m_meshCustomMaterial.albedo_r = albedo.x;
		j.second->m_meshCustomMaterial.albedo_g = albedo.y;
		j.second->m_meshCustomMaterial.albedo_b = albedo.z;
		j.second->m_meshCustomMaterial.metallic = MRAT.x;
		j.second->m_meshCustomMaterial.roughness = MRAT.y;
		j.second->m_meshCustomMaterial.ao = MRAT.z;
		j.second->m_meshCustomMaterial.alpha = albedo.w;
		j.second->m_meshCustomMaterial.thickness = MRAT.w;
	}

	return true;
};

bool GameInstance::setup()
{
	bool l_result = true;
	l_result = l_result && PlayerComponentCollection::setup();
	l_result = l_result && GameInstanceNS::setup();

	return l_result;
}

bool GameInstance::initialize()
{
	bool l_result = true;
	g_pModuleManager->getFileSystem()->loadScene("Res//Scenes//default.InnoScene");

	l_result = l_result && PlayerComponentCollection::initialize();
	l_result = l_result && GameInstanceNS::initialize();

	return l_result;
}

bool GameInstance::update()
{
	return 	GameInstanceNS::update();
}

bool GameInstance::terminate()
{
	GameInstanceNS::m_objectStatus = ObjectStatus::Terminated;
	return true;
}

ObjectStatus GameInstance::getStatus()
{
	return GameInstanceNS::m_objectStatus;
}

std::string GameInstance::getGameName()
{
	return std::string("GameInstance/");
}

bool GameInstanceNS::update()
{
	if (m_objectStatus == ObjectStatus::Activated)
	{
		auto l_tickTime = g_pModuleManager->getTickTime();
		seed += (l_tickTime / 1000.0f);

		auto updateGameTask = g_pModuleManager->getTaskSystem()->submit("PlayerComponentCollectionUpdateTask", [&]()
		{
			auto l_seed = (1.0f - l_tickTime / 100.0f);
			l_seed = l_seed > 0.0f ? l_seed : 0.01f;
			l_seed = l_seed > 0.85f ? 0.85f : l_seed;
			PlayerComponentCollection::update(l_seed);
		});

		updateSpheres();
	}

	return true;
}

void GameInstanceNS::runTest(unsigned int testTime, std::function<bool()> testCase)
{
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "Start test...");
	for (unsigned int i = 0; i < testTime; i++)
	{
		auto l_result = testCase();
		if (!l_result)
		{
			g_pModuleManager->getLogSystem()->printLog(LogType::INNO_WARNING, "Test failure.");
		}
	}
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "Finished test for " + std::to_string(testTime) + " times.");
}

void PlayerComponentCollection::update(float seed)
{
	auto l_currentCameraPos = m_cameraTransformComponent->m_localTransformVector.m_pos;
	auto l_currentCameraRot = m_cameraTransformComponent->m_localTransformVector.m_rot;

	if (m_smoothInterp)
	{
		if (!InnoMath::isCloseEnough(l_currentCameraPos, m_targetCameraPos))
		{
			m_cameraTransformComponent->m_localTransformVector.m_pos = InnoMath::lerp(l_currentCameraPos, m_targetCameraPos, seed);
		}

		if (m_canSlerp)
		{
			if (!InnoMath::isCloseEnough(l_currentCameraRot, m_targetCameraRot))
			{
				m_cameraTransformComponent->m_localTransformVector.m_rot = InnoMath::slerp(l_currentCameraRot, m_targetCameraRot, seed);
			}
		}
	}
	else
	{
		m_cameraTransformComponent->m_localTransformVector.m_pos = m_targetCameraPos;
		m_cameraTransformComponent->m_localTransformVector.m_rot = m_targetCameraRot;
	}
}

void GameInstanceNS::updateSpheres()
{
	for (unsigned int i = 0; i < m_opaqueSphereVisibleComponents.size(); i += 4)
	{
		auto l_albedoFactor1 = (sin(seed / 2.0f + i) + 1.0f) / 2.0f;
		auto l_albedoFactor2 = (sin(seed / 3.0f + i) + 1.0f) / 2.0f;
		auto l_albedoFactor3 = (sin(seed / 5.0f + i) + 1.0f) / 2.0f;

		auto l_albedo1 = vec4(l_albedoFactor1, l_albedoFactor2, l_albedoFactor3, 1.0f);
		auto l_albedo2 = vec4(l_albedoFactor3, l_albedoFactor2, l_albedoFactor1, 1.0f);
		auto l_albedo3 = vec4(l_albedoFactor2, l_albedoFactor3, l_albedoFactor1, 1.0f);
		auto l_albedo4 = vec4(l_albedoFactor2, l_albedoFactor1, l_albedoFactor3, 1.0f);

		auto l_MRATFactor1 = ((sin(seed / 4.0f + i) + 1.0f) / 2.001f);
		auto l_MRATFactor2 = ((sin(seed / 5.0f + i) + 1.0f) / 2.001f);
		auto l_MRATFactor3 = ((sin(seed / 6.0f + i) + 1.0f) / 2.001f);

		updateMaterial(m_opaqueSphereVisibleComponents[i]->m_modelMap, l_albedo1, vec4(l_MRATFactor1, l_MRATFactor2, l_MRATFactor3, 0.0f));
		updateMaterial(m_opaqueSphereVisibleComponents[i + 1]->m_modelMap, l_albedo2, vec4(l_MRATFactor2, l_MRATFactor1, l_MRATFactor3, 0.0f));
		updateMaterial(m_opaqueSphereVisibleComponents[i + 2]->m_modelMap, l_albedo3, vec4(l_MRATFactor3, l_MRATFactor2, l_MRATFactor1, 0.0f));
		updateMaterial(m_opaqueSphereVisibleComponents[i + 3]->m_modelMap, l_albedo4, vec4(l_MRATFactor3, l_MRATFactor1, l_MRATFactor2, 0.0f));
	}

	for (unsigned int i = 0; i < m_transparentSphereVisibleComponents.size(); i++)
	{
		auto l_albedo = InnoMath::HSVtoRGB(vec4((sin(seed / 6.0f + i) * 0.5f + 0.5f) * 360.0f, 1.0f, 1.0f, 0.5f));
		l_albedo.w = sin(seed / 6.0f + i) * 0.5f + 0.5f;
		auto l_MRAT = vec4(0.0f, sin(seed / 4.0f + i) * 0.5f + 0.5f, 1.0f, sin(seed / 5.0f + i) * 0.5f + 0.5f);
		updateMaterial(m_transparentSphereVisibleComponents[i]->m_modelMap, l_albedo, l_MRAT);
	}

	unsigned int l_matrixDim = 8;
	for (unsigned int i = 0; i < l_matrixDim; i++)
	{
		for (unsigned int j = 0; j < l_matrixDim; j++)
		{
			auto l_MRAT = vec4((float)(i + 1) / (float)l_matrixDim, (float)(j + 1) / (float)l_matrixDim, 1.0f, 1.0f);
			updateMaterial(m_referenceSphereVisibleComponents[i * l_matrixDim + j]->m_modelMap, vec4(1.0f, 1.0f, 1.0f, 1.0f), l_MRAT);
		}
	}
}