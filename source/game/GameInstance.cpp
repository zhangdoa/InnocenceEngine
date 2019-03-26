#include "GameInstance.h"

#include "../../engine/system/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

namespace PlayerComponentCollection
{
	bool setup();
	bool initialize();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_cameraParentEntity;

	TransformComponent* m_cameraTransformComponent;
	InputComponent* m_inputComponent;
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

	void updatePlayer();

	void rotateAroundPositiveYAxis(float offset);
	void rotateAroundRightAxis(float offset);

	std::function<void()> f_sceneLoadingCallback;
};

bool PlayerComponentCollection::setup()
{
	f_sceneLoadingCallback = [&]() {
		m_cameraParentEntity = g_pCoreSystem->getGameSystem()->getEntityID("playerCharacterCamera");
		m_cameraTransformComponent = g_pCoreSystem->getGameSystem()->get<TransformComponent>(m_cameraParentEntity);
		m_inputComponent = g_pCoreSystem->getGameSystem()->spawn<InputComponent>(m_cameraParentEntity);
		m_cameraComponent = g_pCoreSystem->getGameSystem()->spawn<CameraComponent>(m_cameraParentEntity);

		m_targetCameraPos = m_cameraTransformComponent->m_localTransformVector.m_pos;
		m_targetCameraRot = m_cameraTransformComponent->m_localTransformVector.m_rot;
		m_targetCameraRotX = vec4(0.0f, 0.0f, 0.0f, 1.0f);
		m_targetCameraRotY = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	};
	g_pCoreSystem->getFileSystem()->addSceneLoadingCallback(&f_sceneLoadingCallback);

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

	return true;
}

bool PlayerComponentCollection::initialize()
{
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_KEY_S, ButtonStatus::PRESSED }, &f_moveForward);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_KEY_W, ButtonStatus::PRESSED }, &f_moveBackward);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_KEY_A, ButtonStatus::PRESSED }, &f_moveLeft);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_KEY_D, ButtonStatus::PRESSED }, &f_moveRight);

	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_KEY_SPACE, ButtonStatus::PRESSED }, &f_speedUp);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_KEY_SPACE, ButtonStatus::RELEASED }, &f_speedDown);

	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_MOUSE_BUTTON_RIGHT, ButtonStatus::PRESSED }, &f_allowMove);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(m_inputComponent, ButtonData{ INNO_MOUSE_BUTTON_RIGHT, ButtonStatus::RELEASED }, &f_forbidMove);
	g_pCoreSystem->getGameSystem()->registerMouseMovementCallback(m_inputComponent, 0, &f_rotateAroundPositiveYAxis);
	g_pCoreSystem->getGameSystem()->registerMouseMovementCallback(m_inputComponent, 1, &f_rotateAroundRightAxis);

	m_cameraComponent->m_FOVX = 60.0f;
	m_cameraComponent->m_WHRatio = 16.0f / 9.0f;
	m_cameraComponent->m_zNear = 0.1f;
	m_cameraComponent->m_zFar = 2000.0f;
	m_cameraComponent->m_drawFrustum = false;
	m_cameraComponent->m_drawAABB = false;

	m_initialMoveSpeed = 0.5f;
	m_moveSpeed = m_initialMoveSpeed;
	m_rotateSpeed = 10.0f;
	m_canMove = false;

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
	float temp = 0.0f;

	bool setup();
	bool initialize();

	void setupSpheres();
	void setupLights();
	void update();
	void updateLights(float seed);
	void updateSpheres(float seed);

	void runTest(unsigned int testTime, std::function<bool()> testCase);
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	InnoFuture<void>* m_asyncTask;
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

	m_objectStatus = ObjectStatus::ALIVE;

	return true;
}

bool GameInstanceNS::initialize()
{
	return true;
}

INNO_GAME_EXPORT bool GameInstance::setup()
{
	bool result = true;
	result = result && PlayerComponentCollection::setup();
	result = result && GameInstanceNS::setup();

	return result;
}

INNO_GAME_EXPORT bool GameInstance::initialize()
{
	bool result = true;
	result = result && PlayerComponentCollection::initialize();
	result = result && GameInstanceNS::initialize();

	return result;
}

INNO_GAME_EXPORT bool GameInstance::update()
{
	GameInstanceNS::update();

	return true;
}

INNO_GAME_EXPORT bool GameInstance::terminate()
{
	GameInstanceNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	return true;
}

INNO_GAME_EXPORT ObjectStatus GameInstance::getStatus()
{
	return GameInstanceNS::m_objectStatus;
}

INNO_GAME_EXPORT std::string GameInstance::getGameName()
{
	return std::string("GameInstance");
}

void GameInstanceNS::update()
{
	// @TODO: getter for config
	auto l_pause = g_pCoreSystem->getGameSystem();
	if (1)
	{
		auto tempTask = g_pCoreSystem->getTaskSystem()->submit([&]()
		{
			temp += 0.02f;
			updateLights(temp);
			updateSpheres(temp);
		});
		m_asyncTask = &tempTask;
	}
	PlayerComponentCollection::updatePlayer();
}

void GameInstanceNS::updateLights(float seed)
{
}

void GameInstanceNS::updateSpheres(float seed)
{
}

void GameInstanceNS::runTest(unsigned int testTime, std::function<bool()> testCase)
{
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "Start test...");
	for (unsigned int i = 0; i < testTime; i++)
	{
		auto l_result = testCase();
		if (!l_result)
		{
			g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "Test failure.");
		}
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_VERBOSE, "Finished test for " + std::to_string(testTime) + " times.");
}

void PlayerComponentCollection::updatePlayer()
{
	auto l_currentCameraPos = m_cameraTransformComponent->m_localTransformVector.m_pos;
	auto l_currentCameraRot = m_cameraTransformComponent->m_localTransformVector.m_rot;

	if (m_smoothInterp)
	{
		if (!InnoMath::isCloseEnough(l_currentCameraPos, m_targetCameraPos))
		{
			m_cameraTransformComponent->m_localTransformVector.m_pos = InnoMath::lerp(l_currentCameraPos, m_targetCameraPos, 0.85f);
		}

		if (m_canSlerp)
		{
			if (!InnoMath::isCloseEnough(l_currentCameraRot, m_targetCameraRot))
			{
				m_cameraTransformComponent->m_localTransformVector.m_rot = InnoMath::slerp(l_currentCameraRot, m_targetCameraRot, 0.8f);
			}
		}
	}
	else
	{
		m_cameraTransformComponent->m_localTransformVector.m_pos = m_targetCameraPos;
		m_cameraTransformComponent->m_localTransformVector.m_rot = m_targetCameraRot;
	}
}