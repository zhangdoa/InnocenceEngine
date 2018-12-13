#include "GameInstance.h"

#include "../../engine/system/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

namespace PlayerComponentCollection
{
	void setup();

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	TransformComponent* m_transformComponent;
	VisibleComponent* m_visibleComponent;
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

	float m_initialSpeed = 0;
	float m_moveSpeed = 0;
	float m_rotateSpeed = 0;
	bool m_canMove = false;

	void move(vec4 direction, float length);

	void rotateAroundPositiveYAxis(float offset);
	void rotateAroundRightAxis(float offset);
};

void PlayerComponentCollection::setup()
{
	m_transformComponent->m_localTransformVector.m_pos = vec4(0.0f, 4.0f, 3.0f, 1.0f);
	/*m_transformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		m_transformComponent->m_localTransformVector.m_rot,
		vec4(0.0f, 1.0f, 0.0f, 0.0f),
		90.0f
	);*/
	m_cameraComponent->m_FOVX = 60.0f;
	m_cameraComponent->m_WHRatio = 16.0f / 9.0f;
	m_cameraComponent->m_zNear = 0.1f;
	m_cameraComponent->m_zFar = 200.0f;
	m_cameraComponent->m_drawFrustum = false;
	m_cameraComponent->m_drawAABB = false;

	m_initialSpeed = 0.05f;
	m_moveSpeed = m_initialSpeed;
	m_rotateSpeed = 4.0f;
	m_canMove = false;

	f_moveForward = [&]() { move(InnoMath::getDirection(direction::FORWARD, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed); };
	f_moveBackward = [&]() { move(InnoMath::getDirection(direction::BACKWARD, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed); };
	f_moveLeft = [&]() { move(InnoMath::getDirection(direction::LEFT, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed); };
	f_moveRight = [&]() { move(InnoMath::getDirection(direction::RIGHT, m_transformComponent->m_localTransformVector.m_rot), m_moveSpeed); };

	f_speedUp = [&]() { m_moveSpeed = m_initialSpeed * 10.0f; };
	f_speedDown = [&]() { m_moveSpeed = m_initialSpeed; };

	f_allowMove = [&]() { m_canMove = true; };
	f_forbidMove = [&]() { m_canMove = false; };

	f_rotateAroundPositiveYAxis = std::bind(&PlayerComponentCollection::rotateAroundPositiveYAxis, std::placeholders::_1);
	f_rotateAroundRightAxis = std::bind(&PlayerComponentCollection::rotateAroundRightAxis, std::placeholders::_1);
}

void PlayerComponentCollection::move(vec4 direction, float length)
{
	if (m_canMove)
	{
		m_transformComponent->m_localTransformVector.m_pos = InnoMath::moveTo(m_transformComponent->m_localTransformVector.m_pos, direction, (float)length);
	}
}

void PlayerComponentCollection::rotateAroundPositiveYAxis(float offset)
{
	if (m_canMove)
	{
		auto dest_rot = InnoMath::rotateInLocal(
			m_transformComponent->m_localTransformVector.m_rot,
			vec4(0.0f, 1.0f, 0.0f, 0.0f),
			(float)((-offset * m_rotateSpeed) / 180.0f)* PI<float>
		);
		m_transformComponent->m_localTransformVector.m_rot = dest_rot.slerp(m_transformComponent->m_localTransformVector.m_rot, dest_rot, 0.5);
	}
}

void PlayerComponentCollection::rotateAroundRightAxis(float offset)
{
	if (m_canMove)
	{
		auto l_right = InnoMath::getDirection(direction::RIGHT, m_transformComponent->m_localTransformVector.m_rot);
		auto dest_rot = InnoMath::rotateInLocal(
			m_transformComponent->m_localTransformVector.m_rot,
			l_right,
			(float)((offset * m_rotateSpeed) / 180.0f)* PI<float>);

		m_transformComponent->m_localTransformVector.m_rot = dest_rot.slerp(m_transformComponent->m_localTransformVector.m_rot, dest_rot, 0.5);
	}
}

namespace GameInstanceNS
{
	// environment capture entity and its components
	EntityID m_EnvironmentCaptureEntity;
	EnvironmentCaptureComponent* m_environmentCaptureComponent;

	// directional light/ sun entity and its components
	EntityID m_directionalLightEntity;
	TransformComponent* m_directionalLightTransformComponent;
	LightComponent* m_directionalLightComponent;

	// landscape entity and its components
	EntityID m_landscapeEntity;
	TransformComponent* m_landscapeTransformComponent;
	VisibleComponent* m_landscapeVisibleComponent;

	// pawn entity 1 and its components
	EntityID m_pawnEntity1;
	TransformComponent* m_pawnTransformComponent1;
	VisibleComponent* m_pawnVisibleComponent1;

	// pawn entity 2 and its components
	EntityID m_pawnEntity2;
	TransformComponent* m_pawnTransformComponent2;
	VisibleComponent* m_pawnVisibleComponent2;

	// sphere entities and their components
	std::vector<EntityID> m_sphereEntitys;
	std::vector<TransformComponent*> m_sphereTransformComponents;
	std::vector<VisibleComponent*> m_sphereVisibleComponents;

	// punctual point light entities and their components
	std::vector<EntityID> m_pointLightEntitys;
	std::vector<TransformComponent*> m_pointLightTransformComponents;
	std::vector<LightComponent*> m_pointLightComponents;
	std::vector<VisibleComponent*> m_pointLightVisibleComponents;

	float temp = 0.0f;

	void setupSpheres();
	void setupLights();
	void updateLights(float seed);
	void updateSpheres(float seed);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	InnoFuture<void>* m_asyncTask;
}

INNO_GAME_EXPORT bool GameInstance::setup()
{
	// setup player character
	PlayerComponentCollection::m_parentEntity = InnoMath::createEntityID();

	PlayerComponentCollection::m_transformComponent = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>(PlayerComponentCollection::m_parentEntity);
	PlayerComponentCollection::m_transformComponent->m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
	PlayerComponentCollection::m_visibleComponent = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>(PlayerComponentCollection::m_parentEntity);
	PlayerComponentCollection::m_inputComponent = g_pCoreSystem->getGameSystem()->spawn<InputComponent>(PlayerComponentCollection::m_parentEntity);
	PlayerComponentCollection::m_cameraComponent = g_pCoreSystem->getGameSystem()->spawn<CameraComponent>(PlayerComponentCollection::m_parentEntity);

	PlayerComponentCollection::setup();

	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_KEY_S, ButtonStatus::PRESSED }, &PlayerComponentCollection::f_moveForward);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_KEY_W, ButtonStatus::PRESSED }, &PlayerComponentCollection::f_moveBackward);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_KEY_A, ButtonStatus::PRESSED }, &PlayerComponentCollection::f_moveLeft);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_KEY_D, ButtonStatus::PRESSED }, &PlayerComponentCollection::f_moveRight);

	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_KEY_SPACE, ButtonStatus::PRESSED }, &PlayerComponentCollection::f_speedUp);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_KEY_SPACE, ButtonStatus::RELEASED }, &PlayerComponentCollection::f_speedDown);

	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_MOUSE_BUTTON_RIGHT, ButtonStatus::PRESSED }, &PlayerComponentCollection::f_allowMove);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, ButtonData{ INNO_MOUSE_BUTTON_RIGHT, ButtonStatus::RELEASED }, &PlayerComponentCollection::f_forbidMove);
	g_pCoreSystem->getGameSystem()->registerMouseMovementCallback(PlayerComponentCollection::m_inputComponent, 0, &PlayerComponentCollection::f_rotateAroundPositiveYAxis);
	g_pCoreSystem->getGameSystem()->registerMouseMovementCallback(PlayerComponentCollection::m_inputComponent, 1, &PlayerComponentCollection::f_rotateAroundRightAxis);

	//setup environment capture component
	GameInstanceNS::m_EnvironmentCaptureEntity = InnoMath::createEntityID();

	GameInstanceNS::m_environmentCaptureComponent = g_pCoreSystem->getGameSystem()->spawn<EnvironmentCaptureComponent>(GameInstanceNS::m_EnvironmentCaptureEntity);
	GameInstanceNS::m_environmentCaptureComponent->m_cubemapTextureFileName = "ibl//Playa_Sunrise.hdr";

	//setup directional light
	GameInstanceNS::m_directionalLightEntity = InnoMath::createEntityID();

	GameInstanceNS::m_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>(GameInstanceNS::m_directionalLightEntity);
	GameInstanceNS::m_directionalLightTransformComponent->m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
	GameInstanceNS::m_directionalLightTransformComponent->m_localTransformVector.m_pos = vec4(0.0f, 4.0f, 0.0f, 1.0f);
	GameInstanceNS::m_directionalLightTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		GameInstanceNS::m_directionalLightTransformComponent->m_localTransformVector.m_rot,
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		-90.0f
	);

	GameInstanceNS::m_directionalLightComponent = g_pCoreSystem->getGameSystem()->spawn<LightComponent>(GameInstanceNS::m_directionalLightEntity);
	GameInstanceNS::m_directionalLightComponent->m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	GameInstanceNS::m_directionalLightComponent->m_lightType = LightType::DIRECTIONAL;
	GameInstanceNS::m_directionalLightComponent->m_drawAABB = false;

	//setup landscape
	//GameInstanceNS::m_landscapeEntity = InnoMath::createEntityID();

	//GameInstanceNS::m_landscapeTransformComponent = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>(GameInstanceNS::m_landscapeEntity);
	//GameInstanceNS::m_landscapeTransformComponent->m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
	//GameInstanceNS::m_landscapeTransformComponent->m_localTransformVector.m_pos = vec4(0.0f, -4.0f, 0.0f, 1.0f);
	//GameInstanceNS::m_landscapeTransformComponent->m_localTransformVector.m_scale = vec4(200.0f, 200.0f, 0.1f, 1.0f);
	//GameInstanceNS::m_landscapeTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
	//	GameInstanceNS::m_landscapeTransformComponent->m_localTransformVector.m_rot,
	//	vec4(1.0f, 0.0f, 0.0f, 0.0f),
	//	90.0f
	//);
	//GameInstanceNS::m_landscapeVisibleComponent = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>(GameInstanceNS::m_landscapeEntity);
	//GameInstanceNS::m_landscapeVisibleComponent->m_visiblilityType = VisiblilityType::STATIC_MESH;
	//GameInstanceNS::m_landscapeVisibleComponent->m_meshShapeType = MeshShapeType::CUBE;

	//setup pawn 1
	GameInstanceNS::m_pawnEntity1 = InnoMath::createEntityID();

	GameInstanceNS::m_pawnTransformComponent1 = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>(GameInstanceNS::m_pawnEntity1);
	GameInstanceNS::m_pawnTransformComponent1->m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
	GameInstanceNS::m_pawnTransformComponent1->m_localTransformVector.m_scale = vec4(0.1f, 0.1f, 0.1f, 1.0f);
	GameInstanceNS::m_pawnVisibleComponent1 = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>(GameInstanceNS::m_pawnEntity1);
	GameInstanceNS::m_pawnVisibleComponent1->m_visiblilityType = VisiblilityType::STATIC_MESH;
	GameInstanceNS::m_pawnVisibleComponent1->m_meshShapeType = MeshShapeType::CUSTOM;
	GameInstanceNS::m_pawnVisibleComponent1->m_meshDrawMethod = MeshPrimitiveTopology::TRIANGLE;
	//GameInstanceNS::m_pawnVisibleComponent1->m_modelFileName = "sponza//sponza.obj";
	//GameInstanceNS::m_pawnVisibleComponent1->m_modelFileName = "cat//cat.obj";
	GameInstanceNS::m_pawnVisibleComponent1->m_textureWrapMethod = TextureWrapMethod::REPEAT;
	GameInstanceNS::m_pawnVisibleComponent1->m_drawAABB = false;

	//setup pawn 2
	GameInstanceNS::m_pawnEntity2 = InnoMath::createEntityID();

	GameInstanceNS::m_pawnTransformComponent2 = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>(GameInstanceNS::m_pawnEntity2);
	GameInstanceNS::m_pawnTransformComponent2->m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
	//GameInstanceNS::m_pawnTransformComponent2->m_localTransformVector.m_scale = vec4(0.01f, 0.01f, 0.01f, 1.0f);
	GameInstanceNS::m_pawnTransformComponent2->m_localTransformVector.m_pos = vec4(0.0f, 0.2f, 3.5f, 1.0f);
	GameInstanceNS::m_pawnVisibleComponent2 = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>(GameInstanceNS::m_pawnEntity2);
	GameInstanceNS::m_pawnVisibleComponent2->m_visiblilityType = VisiblilityType::STATIC_MESH;
	GameInstanceNS::m_pawnVisibleComponent2->m_meshShapeType = MeshShapeType::SPHERE;
	GameInstanceNS::m_pawnVisibleComponent2->m_meshDrawMethod = MeshPrimitiveTopology::TRIANGLE_STRIP;
	GameInstanceNS::m_pawnVisibleComponent2->m_drawAABB = true;
	//GameInstanceNS::m_pawnVisibleComponent2->m_modelFileName = "Orb//Orb.obj";

	GameInstanceNS::setupLights();
	GameInstanceNS::setupSpheres();

	GameInstanceNS::m_objectStatus = ObjectStatus::ALIVE;
	return true;
}

INNO_GAME_EXPORT bool GameInstance::initialize()
{
	return true;
}

INNO_GAME_EXPORT bool GameInstance::update()
{
	auto temp = g_pCoreSystem->getTaskSystem()->submit([]()
	{
		GameInstanceNS::temp += 0.02f;
		GameInstanceNS::updateLights(GameInstanceNS::temp);
		GameInstanceNS::updateSpheres(GameInstanceNS::temp);
	});
	GameInstanceNS::m_asyncTask = &temp;
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

void GameInstanceNS::setupSpheres()
{
	unsigned int sphereMatrixDim = 8;
	float sphereBreadthInterval = 4.0f;
	auto l_containerSize = sphereMatrixDim * sphereMatrixDim;
	m_sphereTransformComponents.reserve(l_containerSize);
	m_sphereVisibleComponents.reserve(l_containerSize);
	m_sphereEntitys.reserve(l_containerSize);
	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_sphereTransformComponents.emplace_back();
		m_sphereVisibleComponents.emplace_back();
		m_sphereEntitys.emplace_back();
	}
	for (unsigned int i = 0; i < m_sphereVisibleComponents.size(); i++)
	{
		m_sphereEntitys[i] = InnoMath::createEntityID();

		m_sphereTransformComponents[i] = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>(m_sphereEntitys[i]);
		m_sphereTransformComponents[i]->m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
		m_sphereTransformComponents[i]->m_localTransformVector.m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_sphereVisibleComponents[i] = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>(m_sphereEntitys[i]);
		m_sphereVisibleComponents[i]->m_visiblilityType = VisiblilityType::STATIC_MESH;
		m_sphereVisibleComponents[i]->m_meshShapeType = MeshShapeType::CUSTOM;
		m_sphereVisibleComponents[i]->m_meshDrawMethod = MeshPrimitiveTopology::TRIANGLE;
		m_sphereVisibleComponents[i]->m_drawAABB = true;
		m_sphereVisibleComponents[i]->m_modelFileName = "Orb//Orb.obj";
	}
	for (unsigned int i = 0; i < sphereMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < sphereMatrixDim; j++)
		{
			m_sphereTransformComponents[i * sphereMatrixDim + j]->m_localTransformVector.m_pos = vec4((-(sphereMatrixDim - 1.0f) * sphereBreadthInterval / 2.0f) + (i * sphereBreadthInterval), 0.0f, (j * sphereBreadthInterval) - 2.0f * (sphereMatrixDim - 1), 1.0f);
		}
	}
}

void GameInstanceNS::setupLights()
{
	unsigned int pointLightMatrixDim = 8;
	float pointLightBreadthInterval = 20.0f;
	auto l_containerSize = pointLightMatrixDim * pointLightMatrixDim;

	m_pointLightTransformComponents.reserve(l_containerSize);
	m_pointLightComponents.reserve(l_containerSize);
	m_pointLightVisibleComponents.reserve(l_containerSize);
	m_pointLightEntitys.reserve(l_containerSize);

	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_pointLightTransformComponents.emplace_back();
		m_pointLightComponents.emplace_back();
		m_pointLightVisibleComponents.emplace_back();
		m_pointLightEntitys.emplace_back();
	}
	for (unsigned int i = 0; i < l_containerSize; i++)
	{
		m_pointLightEntitys[i] = InnoMath::createEntityID();

		m_pointLightTransformComponents[i] = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>(m_pointLightEntitys[i]);
		m_pointLightTransformComponents[i]->m_parentTransformComponent = g_pCoreSystem->getGameSystem()->getRootTransformComponent();
		m_pointLightTransformComponents[i]->m_localTransformVector.m_scale = vec4(0.1f, 0.1f, 0.1f, 1.0f);
		m_pointLightComponents[i] = g_pCoreSystem->getGameSystem()->spawn<LightComponent>(m_pointLightEntitys[i]);
		m_pointLightVisibleComponents[i] = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>(m_pointLightEntitys[i]);
		m_pointLightComponents[i]->m_luminousFlux = 400.0f;
		m_pointLightVisibleComponents[i]->m_visiblilityType = VisiblilityType::EMISSIVE;
		m_pointLightVisibleComponents[i]->m_meshShapeType = MeshShapeType::SPHERE;
		m_pointLightVisibleComponents[i]->m_meshDrawMethod = MeshPrimitiveTopology::TRIANGLE_STRIP;
	}
	for (unsigned int i = 0; i < pointLightMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < pointLightMatrixDim; j++)
		{
			m_pointLightTransformComponents[i * pointLightMatrixDim + j]->m_localTransformVector.m_pos = vec4((-(pointLightMatrixDim - 1.0f) * pointLightBreadthInterval / 2.0f) + (i * pointLightBreadthInterval), 2.0f + (j * pointLightBreadthInterval), 4.0f, 1.0f);
		}
	}
}

void GameInstanceNS::updateLights(float seed)
{
	m_directionalLightTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		m_directionalLightTransformComponent->m_localTransformVector.m_rot,
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		0.2f * (float)sin(seed)
	);

	for (unsigned int i = 0; i < m_pointLightComponents.size(); i += 4)
	{
		auto l_color1 = vec4((sin(seed + i) + 1.0f) * 5.0f / 2.0f, 0.2 * 5.0f, 0.4 * 5.0f, 1.0f);
		auto l_color2 = vec4(0.2 * 5.0f, (sin(seed + i) + 1.0f) * 5.0f / 2.0f, 0.4 * 5.0f, 1.0f);
		auto l_color3 = vec4(0.2 * 5.0f, 0.4 * 5.0f, (sin(seed + i) + 1.0f) * 5.0f / 2.0f, 1.0f);
		auto l_color4 = vec4((sin(seed + i * 2.0f) + 1.0f) * 5.0f / 2.0f, (sin(seed + i * 3.0f) + 1.0f) * 5.0f / 2.0f, (sin(seed + i * 5.0f) + 1.0f) * 5.0f / 2.0f, 1.0f);

		m_pointLightComponents[i]->m_color = l_color1;
		m_pointLightComponents[i + 1]->m_color = l_color2;
		m_pointLightComponents[i + 2]->m_color = l_color3;
		m_pointLightComponents[i + 3]->m_color = l_color4;

		std::function<void(ModelMap modelMap, vec4 albedo)> f_setMeshColor = [&](ModelMap modelMap, vec4 albedo)
		{
			for (auto& j : modelMap)
			{
				j.second->m_meshCustomMaterial.albedo_r = albedo.x;
				j.second->m_meshCustomMaterial.albedo_g = albedo.y;
				j.second->m_meshCustomMaterial.albedo_b = albedo.z;
			}
		};

		f_setMeshColor(m_pointLightVisibleComponents[i]->m_modelMap, l_color1);
		f_setMeshColor(m_pointLightVisibleComponents[i + 1]->m_modelMap, l_color2);
		f_setMeshColor(m_pointLightVisibleComponents[i + 2]->m_modelMap, l_color3);
		f_setMeshColor(m_pointLightVisibleComponents[i + 3]->m_modelMap, l_color4);
	}
}

void GameInstanceNS::updateSpheres(float seed)
{
	//auto l_t = InnoMath::rotateInGlobal(
	//	m_pawnTransformComponent2->m_localTransformVector.m_pos,
	//	m_pawnTransformComponent2->m_globalTransformVector.m_pos,
	//	vec4(0.0f, 1.0f, 0.0f, 0.0f),
	//	0.2f
	//);
	//m_pawnTransformComponent2->m_localTransformVector.m_pos = std::get<0>(l_t);
	//m_pawnTransformComponent2->m_globalTransformVector.m_pos = std::get<1>(l_t);

	//for (unsigned int i = 0; i < m_sphereTransformComponents.size(); i++)
	//{
	//	auto l_t = InnoMath::rotateInLocal(
	//		m_sphereTransformComponents[i]->m_localTransformVector.m_rot,
	//		vec4(0.0f, 1.0f, 0.0f, 0.0f),
	//		0.2f
	//	);
	//	m_sphereTransformComponents[i]->m_localTransformVector.m_rot = l_t;
	//}

	for (unsigned int i = 0; i < m_sphereVisibleComponents.size(); i += 4)
	{
		auto l_albedoFactor1 = (sin(seed / 2.0f + i) + 1.0f) / 2.0f;
		auto l_albedoFactor2 = (sin(seed / 3.0f + i) + 1.0f) / 2.0f;
		auto l_albedoFactor3 = (sin(seed / 5.0f + i) + 1.0f) / 2.0f;

		auto l_albedo1 = vec4(l_albedoFactor1, l_albedoFactor2, l_albedoFactor3, 1.0f);
		auto l_albedo2 = vec4(l_albedoFactor3, l_albedoFactor2, l_albedoFactor1, 1.0f);
		auto l_albedo3 = vec4(l_albedoFactor2, l_albedoFactor3, l_albedoFactor1, 1.0f);
		auto l_albedo4 = vec4(l_albedoFactor2, l_albedoFactor1, l_albedoFactor3, 1.0f);

		auto l_MRAFactor1 = ((sin(seed / 4.0f + i) + 1.0f) / 2.001f);
		auto l_MRAFactor2 = ((sin(seed / 5.0f + i) + 1.0f) / 2.001f);
		auto l_MRAFactor3 = ((sin(seed / 6.0f + i) + 1.0f) / 2.001f);

		std::function<void(ModelMap modelMap, vec4 albedo, vec4 MRA)> f_setMRA = [&](ModelMap modelMap, vec4 albedo, vec4 MRA)
		{
			for (auto& j : modelMap)
			{
				j.second->m_meshCustomMaterial.albedo_r = albedo.x;
				j.second->m_meshCustomMaterial.albedo_g = albedo.y;
				j.second->m_meshCustomMaterial.albedo_b = albedo.z;
				j.second->m_meshCustomMaterial.metallic = MRA.x;
				j.second->m_meshCustomMaterial.roughness = MRA.y;
				j.second->m_meshCustomMaterial.ao = MRA.z;
			}
		};

		f_setMRA(m_sphereVisibleComponents[i]->m_modelMap, l_albedo1, vec4(l_MRAFactor1, l_MRAFactor2, l_MRAFactor3, 0.0f));
		f_setMRA(m_sphereVisibleComponents[i + 1]->m_modelMap, l_albedo2, vec4(l_MRAFactor2, l_MRAFactor1, l_MRAFactor3, 0.0f));
		f_setMRA(m_sphereVisibleComponents[i + 2]->m_modelMap, l_albedo3, vec4(l_MRAFactor3, l_MRAFactor2, l_MRAFactor1, 0.0f));
		f_setMRA(m_sphereVisibleComponents[i + 3]->m_modelMap, l_albedo4, vec4(l_MRAFactor3, l_MRAFactor1, l_MRAFactor2, 0.0f));
		f_setMRA(m_pawnVisibleComponent2->m_modelMap, vec4(1.0f, 1.0f, 1.0f, 1.0f), vec4(0.999f, 0.001f, 1.0f, 0.0f));
	}

}
