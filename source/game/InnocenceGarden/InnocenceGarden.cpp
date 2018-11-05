#include "InnocenceGarden.h"
#include "../../engine/exports/HighLevelSystem_Export.h"
#include "../../engine/system/HighLevelSystem/GameSystem.h"
#include "PlayerCharacter.h"

namespace InnocenceGarden
{
	// root entity and its components
	EntityID m_rootEntity;
	TransformComponent* m_rootTransformComponent;

	// player character
	EntityID m_playerEntity;
	PlayerComponent* m_playerComponent;

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

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

void InnocenceGarden::setup()
{
	// setup root entity
	m_rootTransformComponent = InnoGameSystem::spawn<TransformComponent>();
	m_rootTransformComponent->m_parentTransformComponent = nullptr;

	m_rootEntity = InnoMath::createEntityID();
	m_rootTransformComponent->m_parentEntity = m_rootEntity;

	// setup player character
	m_playerComponent = InnoGameSystem::spawn<PlayerComponent>();
	m_playerComponent->m_transformComponent = InnoGameSystem::spawn<TransformComponent>();
	m_playerComponent->m_transformComponent->m_parentTransformComponent = m_rootTransformComponent;
	m_playerComponent->m_visibleComponent = InnoGameSystem::spawn<VisibleComponent>();
	m_playerComponent->m_inputComponent = InnoGameSystem::spawn<InputComponent>();
	m_playerComponent->m_cameraComponent = InnoGameSystem::spawn<CameraComponent>();

	m_playerComponent->setup();
	m_playerComponent->m_transformComponent->m_parentTransformComponent = m_rootTransformComponent;
	m_playerComponent->m_transformComponent->m_localTransformVector.m_pos = vec4(0.0f, 4.0f, 3.0f, 1.0f);

	m_playerComponent->m_cameraComponent->m_drawFrustum = false;
	m_playerComponent->m_cameraComponent->m_drawAABB = false;
	InnoGameSystem::registerButtonStatusCallback(m_playerComponent->m_inputComponent, button{ INNO_KEY_S, buttonStatus::PRESSED }, &m_playerComponent->f_moveForward);
	InnoGameSystem::registerButtonStatusCallback(m_playerComponent->m_inputComponent, button{ INNO_KEY_W, buttonStatus::PRESSED }, &m_playerComponent->f_moveBackward);
	InnoGameSystem::registerButtonStatusCallback(m_playerComponent->m_inputComponent, button{ INNO_KEY_A, buttonStatus::PRESSED }, &m_playerComponent->f_moveLeft);
	InnoGameSystem::registerButtonStatusCallback(m_playerComponent->m_inputComponent, button{ INNO_KEY_D, buttonStatus::PRESSED }, &m_playerComponent->f_moveRight);
	InnoGameSystem::registerButtonStatusCallback(m_playerComponent->m_inputComponent, button{ INNO_MOUSE_BUTTON_RIGHT, buttonStatus::PRESSED }, &m_playerComponent->f_allowMove);
	InnoGameSystem::registerButtonStatusCallback(m_playerComponent->m_inputComponent, button{ INNO_MOUSE_BUTTON_RIGHT, buttonStatus::RELEASED }, &m_playerComponent->f_forbidMove);
	InnoGameSystem::registerMouseMovementCallback(m_playerComponent->m_inputComponent, 0, &m_playerComponent->f_rotateAroundPositiveYAxis);
	InnoGameSystem::registerMouseMovementCallback(m_playerComponent->m_inputComponent, 1, &m_playerComponent->f_rotateAroundRightAxis);

	m_playerEntity = InnoMath::createEntityID();
	m_playerComponent->m_parentEntity = m_playerEntity;

	//setup environment capture component
	m_environmentCaptureComponent = InnoGameSystem::spawn<EnvironmentCaptureComponent>();
	m_environmentCaptureComponent->m_cubemapTextureFileName = "ibl//Playa_Sunrise.hdr";

	m_EnvironmentCaptureEntity = InnoMath::createEntityID();

	m_environmentCaptureComponent->m_parentEntity = m_EnvironmentCaptureEntity;

	//setup directional light
	m_directionalLightTransformComponent = InnoGameSystem::spawn<TransformComponent>();
	m_directionalLightTransformComponent->m_parentTransformComponent = m_rootTransformComponent;
	m_directionalLightTransformComponent->m_localTransformVector.m_pos = vec4(0.0f, 4.0f, 0.0f, 1.0f);
	m_directionalLightTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		m_directionalLightTransformComponent->m_localTransformVector.m_rot,
		vec4(-1.0f, 0.0f, 0.0f, 0.0f),
		35.0f
	);
	m_directionalLightComponent = InnoGameSystem::spawn<LightComponent>();
	m_directionalLightComponent->m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_directionalLightComponent->m_lightType = lightType::DIRECTIONAL;
	m_directionalLightComponent->m_drawAABB = false;

	m_directionalLightEntity = InnoMath::createEntityID();

	m_directionalLightTransformComponent->m_parentEntity = m_directionalLightEntity;
	m_directionalLightComponent->m_parentEntity = m_directionalLightEntity;

	//setup landscape
	m_landscapeTransformComponent = InnoGameSystem::spawn<TransformComponent>();
	m_landscapeTransformComponent->m_parentTransformComponent = m_rootTransformComponent;
	m_landscapeTransformComponent->m_localTransformVector.m_scale = vec4(20.0f, 20.0f, 0.1f, 1.0f);
	m_landscapeTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		m_directionalLightTransformComponent->m_localTransformVector.m_rot,
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		90.0f
	);
	m_landscapeVisibleComponent = InnoGameSystem::spawn<VisibleComponent>();
	m_landscapeVisibleComponent->m_visiblilityType = visiblilityType::STATIC_MESH;
	m_landscapeVisibleComponent->m_meshShapeType = meshShapeType::CUBE;

	m_landscapeEntity = InnoMath::createEntityID();

	m_landscapeTransformComponent->m_parentEntity = m_landscapeEntity;
	m_landscapeVisibleComponent->m_parentEntity = m_landscapeEntity;

	setupLights();
	setupSpheres();

	//setup pawn 1
	m_pawnTransformComponent1 = InnoGameSystem::spawn<TransformComponent>();
	m_pawnTransformComponent1->m_parentTransformComponent = m_rootTransformComponent;
	m_pawnTransformComponent1->m_localTransformVector.m_scale = vec4(0.1f, 0.1f, 0.1f, 1.0f);
	m_pawnVisibleComponent1 = InnoGameSystem::spawn<VisibleComponent>();
	m_pawnVisibleComponent1->m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnVisibleComponent1->m_meshShapeType = meshShapeType::CUSTOM;
	m_pawnVisibleComponent1->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	//m_pawnVisibleComponent1->m_modelFileName = "sponza//sponza.obj";
	//m_pawnVisibleComponent1->m_modelFileName = "cat//cat.obj";
	m_pawnVisibleComponent1->m_textureWrapMethod = textureWrapMethod::REPEAT;
	m_pawnVisibleComponent1->m_drawAABB = false;
	m_pawnVisibleComponent1->m_useTexture = true;
	m_pawnVisibleComponent1->m_albedo = vec4(0.95f, 0.93f, 0.88f, 1.0f);
	m_pawnVisibleComponent1->m_MRA = vec4(0.0f, 0.35f, 1.0f, 1.0f);

	m_pawnEntity1 = InnoMath::createEntityID();

	m_pawnTransformComponent1->m_parentEntity = m_pawnEntity1;
	m_pawnVisibleComponent1->m_parentEntity = m_pawnEntity1;

	//setup pawn 2
	m_pawnTransformComponent2 = InnoGameSystem::spawn<TransformComponent>();
	m_pawnTransformComponent2->m_parentTransformComponent = m_rootTransformComponent;
	m_pawnTransformComponent2->m_localTransformVector.m_scale = vec4(0.01f, 0.01f, 0.01f, 1.0f);
	m_pawnTransformComponent2->m_localTransformVector.m_pos = vec4(0.0f, 0.2f, 3.5f, 1.0f);
	m_pawnVisibleComponent2 = InnoGameSystem::spawn<VisibleComponent>();
	m_pawnVisibleComponent2->m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnVisibleComponent2->m_meshShapeType = meshShapeType::CUSTOM;
	m_pawnVisibleComponent2->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	m_pawnVisibleComponent2->m_drawAABB = true;
	m_pawnVisibleComponent2->m_modelFileName = "lantern//lantern.obj";

	m_pawnEntity2 = InnoMath::createEntityID();

	m_pawnTransformComponent2->m_parentEntity = m_pawnEntity2;
	m_pawnVisibleComponent2->m_parentEntity = m_pawnEntity2;

	m_objectStatus = objectStatus::ALIVE;
}

void InnocenceGarden::initialize()
{
}

void InnocenceGarden::update()
{
	temp += 0.02f;
	updateLights(temp);
	updateSpheres(temp);
}

void InnocenceGarden::terminate()
{
	m_objectStatus = objectStatus::SHUTDOWN;
}

std::string InnocenceGarden::getGameName()
{
	return std::string("InnocenceGarden");
}

void InnocenceGarden::setupSpheres()
{
	unsigned int sphereMatrixDim = 8;
	float sphereBreadthInterval = 4.0f;
	auto l_containerSize = sphereMatrixDim * sphereMatrixDim;
	m_sphereTransformComponents.reserve(l_containerSize);
	m_sphereVisibleComponents.reserve(l_containerSize);
	m_sphereEntitys.reserve(l_containerSize);
	for (auto i = (unsigned int)0; i < l_containerSize; i++)
	{
		m_sphereTransformComponents.emplace_back();
		m_sphereVisibleComponents.emplace_back();
		m_sphereEntitys.emplace_back();
	}
	for (auto i = (unsigned int)0; i < m_sphereVisibleComponents.size(); i++)
	{
		m_sphereTransformComponents[i] = InnoGameSystem::spawn<TransformComponent>();
		m_sphereTransformComponents[i]->m_parentTransformComponent = m_rootTransformComponent;
		m_sphereTransformComponents[i]->m_localTransformVector.m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_sphereVisibleComponents[i] = InnoGameSystem::spawn<VisibleComponent>();
		m_sphereVisibleComponents[i]->m_visiblilityType = visiblilityType::STATIC_MESH;
		m_sphereVisibleComponents[i]->m_meshShapeType = meshShapeType::CUSTOM;
		m_sphereVisibleComponents[i]->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
		m_sphereVisibleComponents[i]->m_drawAABB = true;
		m_sphereVisibleComponents[i]->m_modelFileName = "Orb//Orb.obj";
		m_sphereVisibleComponents[i]->m_useTexture = true;

		m_sphereEntitys[i] = InnoMath::createEntityID();

		m_sphereTransformComponents[i]->m_parentEntity = m_sphereEntitys[i];
		m_sphereVisibleComponents[i]->m_parentEntity = m_sphereEntitys[i];
	}
	for (auto i = (unsigned int)0; i < m_sphereVisibleComponents.size(); i += 4)
	{
		//Copper
		//m_sphereVisibleComponents[i]->m_albedo = vec4(0.95, 0.64, 0.54, 1.0f);
		////Gold
		//m_sphereVisibleComponents[i + 1]->m_albedo = vec4(1.0f0, 0.71, 0.29, 1.0f);
		////Iron
		//m_sphereVisibleComponents[i + 2]->m_albedo = vec4(0.56, 0.57, 0.58, 1.0f);
		////Silver
		//m_sphereVisibleComponents[i + 3]->m_albedo = vec4(0.95, 0.93, 0.88, 1.0f);
	}
	for (auto i = (unsigned int)0; i < sphereMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < sphereMatrixDim; j++)
		{
			m_sphereTransformComponents[i * sphereMatrixDim + j]->m_localTransformVector.m_pos = vec4((-(sphereMatrixDim - 1.0f) * sphereBreadthInterval / 2.0f) + (i * sphereBreadthInterval), 0.0f, (j * sphereBreadthInterval) - 2.0f * (sphereMatrixDim - 1), 1.0f);
			m_sphereVisibleComponents[i * sphereMatrixDim + j]->m_MRA = vec4((float)(i) / (float)(sphereMatrixDim), (float)(j) / (float)(sphereMatrixDim), 1.0f, 1.0f);
		}
	}
}

void InnocenceGarden::setupLights()
{
	unsigned int pointLightMatrixDim = 8;
	float pointLightBreadthInterval = 4.0f;
	auto l_containerSize = pointLightMatrixDim * pointLightMatrixDim;
	m_sphereTransformComponents.reserve(l_containerSize);
	m_sphereVisibleComponents.reserve(l_containerSize);
	m_sphereEntitys.reserve(l_containerSize);
	for (auto i = (unsigned int)0; i < l_containerSize; i++)
	{
		m_pointLightTransformComponents.emplace_back();
		m_pointLightComponents.emplace_back();
		m_pointLightVisibleComponents.emplace_back();
		m_pointLightEntitys.emplace_back();
	}
	for (auto i = (unsigned int)0; i < m_pointLightComponents.size(); i++)
	{
		m_pointLightTransformComponents[i] = InnoGameSystem::spawn<TransformComponent>();
		m_pointLightTransformComponents[i]->m_parentTransformComponent = m_rootTransformComponent;
		m_pointLightTransformComponents[i]->m_localTransformVector.m_scale = vec4(0.1f, 0.1f, 0.1f, 1.0f);
		m_pointLightComponents[i] = InnoGameSystem::spawn<LightComponent>();
		m_pointLightVisibleComponents[i] = InnoGameSystem::spawn<VisibleComponent>();
		m_pointLightVisibleComponents[i]->m_visiblilityType = visiblilityType::EMISSIVE;
		m_pointLightVisibleComponents[i]->m_meshShapeType = meshShapeType::SPHERE;
		m_pointLightVisibleComponents[i]->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
		m_pointLightVisibleComponents[i]->m_useTexture = false;

		m_pointLightEntitys[i] = InnoMath::createEntityID();

		m_pointLightTransformComponents[i]->m_parentEntity = m_pointLightEntitys[i];
		m_pointLightComponents[i]->m_parentEntity = m_pointLightEntitys[i];
		m_pointLightVisibleComponents[i]->m_parentEntity = m_pointLightEntitys[i];
	}
	for (auto i = (unsigned int)0; i < pointLightMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < pointLightMatrixDim; j++)
		{
			m_pointLightTransformComponents[i * pointLightMatrixDim + j]->m_localTransformVector.m_pos = vec4((-(pointLightMatrixDim - 1.0f) * pointLightBreadthInterval / 2.0f) + (i * pointLightBreadthInterval), 2.0f + (j * pointLightBreadthInterval), 4.0f, 1.0f);
		}
	}
}

void InnocenceGarden::updateLights(float seed)
{
	m_landscapeTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		m_directionalLightTransformComponent->m_localTransformVector.m_rot,
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		0.2f
	);
	for (auto i = (unsigned int)0; i < m_pointLightComponents.size(); i += 4)
	{
		m_pointLightVisibleComponents[i]->m_albedo = vec4((sin(seed + i) + 1.0f) * 5.0f / 2.0f, 0.2 * 5.0f, 0.4 * 5.0f, 1.0f);
		m_pointLightComponents[i]->m_color = vec4((sin(seed + i) + 1.0f) * 5.0f / 2.0f, 0.2 * 5.0f, 0.4 * 5.0f, 1.0f);
		m_pointLightVisibleComponents[i + 1]->m_albedo = vec4(0.2 * 5.0f, (sin(seed + i) + 1.0f) * 5.0f / 2.0f, 0.4 * 5.0f, 1.0f);
		m_pointLightComponents[i + 1]->m_color = vec4(0.2 * 5.0f, (sin(seed + i) + 1.0f) * 5.0f / 2.0f, 0.4 * 5.0f, 1.0f);
		m_pointLightVisibleComponents[i + 2]->m_albedo = vec4(0.2 * 5.0f, 0.4 * 5.0f, (sin(seed + i) + 1.0f) * 5.0f / 2.0f, 1.0f);
		m_pointLightComponents[i + 2]->m_color = vec4(0.2 * 5.0f, 0.4 * 5.0f, (sin(seed + i) + 1.0f) * 5.0f / 2.0f, 1.0f);
		m_pointLightVisibleComponents[i + 3]->m_albedo = vec4((sin(seed + i * 2.0f) + 1.0f) * 5.0f / 2.0f, (sin(seed + i * 3.0f) + 1.0f) * 5.0f / 2.0f, (sin(seed + i * 5.0f) + 1.0f) * 5.0f / 2.0f, 1.0f);
		m_pointLightComponents[i + 3]->m_color = vec4((sin(seed + i * 2.0f) + 1.0f) * 5.0f / 2.0f, (sin(seed + i * 3.0f) + 1.0f) * 5.0f / 2.0f, (sin(seed + i * 5.0f) + 1.0f) * 5.0f / 2.0f, 1.0f);
	}
}

void InnocenceGarden::updateSpheres(float seed)
{
	auto l_t = InnoMath::rotateInGlobal(
		m_pawnTransformComponent2->m_localTransformVector.m_pos,
		m_pawnTransformComponent2->m_globalTransformVector.m_pos,
		vec4(0.0f, 1.0f, 0.0f, 0.0f),
		0.2f
	);
	m_pawnTransformComponent2->m_localTransformVector.m_pos = std::get<0>(l_t);
	m_pawnTransformComponent2->m_globalTransformVector.m_pos = std::get<1>(l_t);
}
