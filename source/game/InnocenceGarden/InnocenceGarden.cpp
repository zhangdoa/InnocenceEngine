#include "InnocenceGarden.h"
#include "../../engine/exports/LowLevelSystem_Export.h"
#include "../../engine/exports/HighLevelSystem_Export.h"
#include "../../engine/system/LowLevelSystem/MemorySystem.h"
#include "../../engine/system/HighLevelSystem/GameSystem.h"
#include "PlayerCharacter.h"

namespace InnocenceGarden
{
	// root entity and its components
	EntityID m_rootEntity;
	TransformComponent* m_rootTransformComponent;

	// player character
	EntityID m_playerCharacterEntity;
	PlayerCharacter* m_playerCharacterComponent;

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

	double temp = 0.0f;

	void setupSpheres();
	void setupLights();
	void updateLights(double seed);
	void updateSpheres(double seed);

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

void InnocenceGarden::setup()
{
	// setup root entity
	m_rootTransformComponent = InnoMemorySystem::spawn<TransformComponent>();
	m_rootTransformComponent->m_transformVector.m_parentTransform = nullptr;

	InnoGameSystem::addTransformComponent(m_rootTransformComponent);
	m_rootEntity = InnoMath::createEntityID();
	m_rootTransformComponent->m_parentEntity = m_rootEntity;

	// setup player character
	m_playerCharacterComponent = InnoMemorySystem::spawn<PlayerCharacter>();
	m_playerCharacterComponent->setup();
	m_playerCharacterComponent->getTransformComponent().m_transformVector.m_parentTransform = &m_rootTransformComponent->m_transformVector;
	m_playerCharacterComponent->getTransformComponent().m_transformVector.setLocalPos(vec4(0.0, 4.0, 3.0, 1.0));

	m_playerCharacterComponent->getCameraComponent().m_drawFrustum = false;
	m_playerCharacterComponent->getCameraComponent().m_drawAABB = false;
	InnoGameSystem::registerButtonStatusCallback(&m_playerCharacterComponent->getInputComponent(), button{ INNO_KEY_S, buttonStatus::PRESSED }, &m_playerCharacterComponent->f_moveForward);
	InnoGameSystem::registerButtonStatusCallback(&m_playerCharacterComponent->getInputComponent(), button{ INNO_KEY_W, buttonStatus::PRESSED }, &m_playerCharacterComponent->f_moveBackward);
	InnoGameSystem::registerButtonStatusCallback(&m_playerCharacterComponent->getInputComponent(), button{ INNO_KEY_A, buttonStatus::PRESSED }, &m_playerCharacterComponent->f_moveLeft);
	InnoGameSystem::registerButtonStatusCallback(&m_playerCharacterComponent->getInputComponent(), button{ INNO_KEY_D, buttonStatus::PRESSED }, &m_playerCharacterComponent->f_moveRight);
	InnoGameSystem::registerButtonStatusCallback(&m_playerCharacterComponent->getInputComponent(), button{ INNO_MOUSE_BUTTON_RIGHT, buttonStatus::PRESSED }, &m_playerCharacterComponent->f_allowMove);
	InnoGameSystem::registerButtonStatusCallback(&m_playerCharacterComponent->getInputComponent(), button{ INNO_MOUSE_BUTTON_RIGHT, buttonStatus::RELEASED }, &m_playerCharacterComponent->f_forbidMove);
	InnoGameSystem::registerMouseMovementCallback(&m_playerCharacterComponent->getInputComponent(), 0, &m_playerCharacterComponent->f_rotateAroundPositiveYAxis);
	InnoGameSystem::registerMouseMovementCallback(&m_playerCharacterComponent->getInputComponent(), 1, &m_playerCharacterComponent->f_rotateAroundRightAxis);

	InnoGameSystem::addTransformComponent(&m_playerCharacterComponent->getTransformComponent());
	InnoGameSystem::addCameraComponent(&m_playerCharacterComponent->getCameraComponent());
	InnoGameSystem::addInputComponent(&m_playerCharacterComponent->getInputComponent());

	m_playerCharacterEntity = InnoMath::createEntityID();
	m_playerCharacterComponent->m_parentEntity = m_playerCharacterEntity;

	//setup environment capture component
	m_environmentCaptureComponent = InnoMemorySystem::spawn<EnvironmentCaptureComponent>();
	m_environmentCaptureComponent->m_cubemapTextureFileName = "ibl//Playa_Sunrise.hdr";

	m_EnvironmentCaptureEntity = InnoMath::createEntityID();

	m_environmentCaptureComponent->m_parentEntity = m_EnvironmentCaptureEntity;

	InnoGameSystem::addEnvironmentCaptureComponent(m_environmentCaptureComponent);

	//setup directional light
	m_directionalLightTransformComponent = InnoMemorySystem::spawn<TransformComponent>();
	m_directionalLightTransformComponent->m_transformVector.m_parentTransform = &m_rootTransformComponent->m_transformVector;
	m_directionalLightTransformComponent->m_transformVector.setLocalPos(vec4(0.0, 4.0, 0.0, 1.0));
	m_directionalLightTransformComponent->m_transformVector.rotateInLocal(vec4(-1.0, 0.0, 0.0, 0.0), 35.0);
	//m_directionalLightTransformComponent->m_transform.rotateInLocal(vec4(0.0, 1.0, 0.0, 0.0), 35.0);
	m_directionalLightComponent = InnoMemorySystem::spawn<LightComponent>();
	m_directionalLightComponent->m_color = vec4(1.0, 1.0, 1.0, 1.0);
	m_directionalLightComponent->m_lightType = lightType::DIRECTIONAL;
	m_directionalLightComponent->m_drawAABB = false;

	m_directionalLightEntity = InnoMath::createEntityID();

	m_directionalLightTransformComponent->m_parentEntity = m_directionalLightEntity;
	m_directionalLightComponent->m_parentEntity = m_directionalLightEntity;

	InnoGameSystem::addTransformComponent(m_directionalLightTransformComponent);
	InnoGameSystem::addLightComponent(m_directionalLightComponent);

	//setup landscape
	m_landscapeTransformComponent = InnoMemorySystem::spawn<TransformComponent>();
	m_landscapeTransformComponent->m_transformVector.m_parentTransform = &m_rootTransformComponent->m_transformVector;
	m_landscapeTransformComponent->m_transformVector.setLocalScale(vec4(20.0, 20.0, 0.1, 1.0));
	m_landscapeTransformComponent->m_transformVector.rotateInLocal(vec4(1.0, 0.0, 0.0, 0.0), 90.0);
	m_landscapeVisibleComponent = InnoMemorySystem::spawn<VisibleComponent>();
	m_landscapeVisibleComponent->m_visiblilityType = visiblilityType::STATIC_MESH;
	m_landscapeVisibleComponent->m_meshShapeType = meshShapeType::CUBE;

	m_landscapeEntity = InnoMath::createEntityID();

	m_landscapeTransformComponent->m_parentEntity = m_landscapeEntity;
	m_landscapeVisibleComponent->m_parentEntity = m_landscapeEntity;

	InnoGameSystem::addTransformComponent(m_landscapeTransformComponent);
	InnoGameSystem::addVisibleComponent(m_landscapeVisibleComponent);

	setupLights();
	setupSpheres();

	//setup pawn 1
	m_pawnTransformComponent1 = InnoMemorySystem::spawn<TransformComponent>();
	m_pawnTransformComponent1->m_transformVector.m_parentTransform = &m_rootTransformComponent->m_transformVector;
	m_pawnTransformComponent1->m_transformVector.setLocalScale(vec4(0.1, 0.1, 0.1, 1.0));
	m_pawnVisibleComponent1 = InnoMemorySystem::spawn<VisibleComponent>();
	m_pawnVisibleComponent1->m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnVisibleComponent1->m_meshShapeType = meshShapeType::CUSTOM;
	m_pawnVisibleComponent1->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	//m_pawnVisibleComponent1->m_modelFileName = "sponza//sponza.obj";
	//m_pawnVisibleComponent1->m_modelFileName = "cat//cat.obj";
	m_pawnVisibleComponent1->m_textureWrapMethod = textureWrapMethod::REPEAT;
	m_pawnVisibleComponent1->m_drawAABB = false;
	m_pawnVisibleComponent1->m_useTexture = true;
	m_pawnVisibleComponent1->m_albedo = vec4(0.95, 0.93, 0.88, 1.0);
	m_pawnVisibleComponent1->m_MRA = vec4(0.0, 0.35, 1.0, 1.0);

	m_pawnEntity1 = InnoMath::createEntityID();

	m_pawnTransformComponent1->m_parentEntity = m_pawnEntity1;
	m_pawnVisibleComponent1->m_parentEntity = m_pawnEntity1;

	InnoGameSystem::addTransformComponent(m_pawnTransformComponent1);
	InnoGameSystem::addVisibleComponent(m_pawnVisibleComponent1);

	//setup pawn 2
	m_pawnTransformComponent2 = InnoMemorySystem::spawn<TransformComponent>();
	m_pawnTransformComponent2->m_transformVector.m_parentTransform = &m_rootTransformComponent->m_transformVector;
	m_pawnTransformComponent2->m_transformVector.setLocalScale(vec4(0.01, 0.01, 0.01, 1.0));
	m_pawnTransformComponent2->m_transformVector.setLocalPos(vec4(0.0, 0.2, 3.5, 1.0));
	m_pawnVisibleComponent2 = InnoMemorySystem::spawn<VisibleComponent>();
	m_pawnVisibleComponent2->m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnVisibleComponent2->m_meshShapeType = meshShapeType::CUSTOM;
	m_pawnVisibleComponent2->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	m_pawnVisibleComponent2->m_drawAABB = true;
	m_pawnVisibleComponent2->m_modelFileName = "lantern//lantern.obj";

	m_pawnEntity2 = InnoMath::createEntityID();

	m_pawnTransformComponent2->m_parentEntity = m_pawnEntity2;
	m_pawnVisibleComponent2->m_parentEntity = m_pawnEntity2;

	InnoGameSystem::addTransformComponent(m_pawnTransformComponent2);
	InnoGameSystem::addVisibleComponent(m_pawnVisibleComponent2);

	m_objectStatus = objectStatus::ALIVE;
}

void InnocenceGarden::initialize()
{
}

void InnocenceGarden::update()
{
	temp += 0.02;
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
	double sphereBreadthInterval = 4.0;
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
		m_sphereTransformComponents[i] = InnoMemorySystem::spawn<TransformComponent>();
		m_sphereTransformComponents[i]->m_transformVector.m_parentTransform = &m_rootTransformComponent->m_transformVector;
		m_sphereTransformComponents[i]->m_transformVector.setLocalScale(vec4(1.0, 1.0, 1.0, 1.0));
		m_sphereVisibleComponents[i] = InnoMemorySystem::spawn<VisibleComponent>();
		m_sphereVisibleComponents[i]->m_visiblilityType = visiblilityType::STATIC_MESH;
		m_sphereVisibleComponents[i]->m_meshShapeType = meshShapeType::CUSTOM;
		m_sphereVisibleComponents[i]->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
		m_sphereVisibleComponents[i]->m_drawAABB = true;
		m_sphereVisibleComponents[i]->m_modelFileName = "Orb//Orb.obj";
		m_sphereVisibleComponents[i]->m_useTexture = true;

		m_sphereEntitys[i] = InnoMath::createEntityID();

		m_sphereTransformComponents[i]->m_parentEntity = m_sphereEntitys[i];
		m_sphereVisibleComponents[i]->m_parentEntity = m_sphereEntitys[i];

		InnoGameSystem::addTransformComponent(m_sphereTransformComponents[i]);
		InnoGameSystem::addVisibleComponent(m_sphereVisibleComponents[i]);
	}
	for (auto i = (unsigned int)0; i < m_sphereVisibleComponents.size(); i += 4)
	{
		//Copper
		//m_sphereVisibleComponents[i]->m_albedo = vec4(0.95, 0.64, 0.54, 1.0);
		////Gold
		//m_sphereVisibleComponents[i + 1]->m_albedo = vec4(1.00, 0.71, 0.29, 1.0);
		////Iron
		//m_sphereVisibleComponents[i + 2]->m_albedo = vec4(0.56, 0.57, 0.58, 1.0);
		////Silver
		//m_sphereVisibleComponents[i + 3]->m_albedo = vec4(0.95, 0.93, 0.88, 1.0);
	}
	for (auto i = (unsigned int)0; i < sphereMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < sphereMatrixDim; j++)
		{
			m_sphereTransformComponents[i * sphereMatrixDim + j]->m_transformVector.setLocalPos(vec4((-(sphereMatrixDim - 1.0) * sphereBreadthInterval / 2.0) + (i * sphereBreadthInterval), 0.0, (j * sphereBreadthInterval) - 2.0 * (sphereMatrixDim - 1), 1.0));
			m_sphereVisibleComponents[i * sphereMatrixDim + j]->m_MRA = vec4((double)(i) / (double)(sphereMatrixDim), (double)(j) / (double)(sphereMatrixDim), 1.0, 1.0);
		}
	}
}

void InnocenceGarden::setupLights()
{
	unsigned int pointLightMatrixDim = 8;
	double pointLightBreadthInterval = 4.0;
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
		m_pointLightTransformComponents[i] = InnoMemorySystem::spawn<TransformComponent>();
		m_pointLightTransformComponents[i]->m_transformVector.m_parentTransform = &m_rootTransformComponent->m_transformVector;
		m_pointLightTransformComponents[i]->m_transformVector.setLocalScale(vec4(0.1, 0.1, 0.1, 1.0));
		m_pointLightComponents[i] = InnoMemorySystem::spawn<LightComponent>();
		m_pointLightVisibleComponents[i] = InnoMemorySystem::spawn<VisibleComponent>();
		m_pointLightVisibleComponents[i]->m_visiblilityType = visiblilityType::EMISSIVE;
		m_pointLightVisibleComponents[i]->m_meshShapeType = meshShapeType::SPHERE;
		m_pointLightVisibleComponents[i]->m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
		m_pointLightVisibleComponents[i]->m_useTexture = false;

		m_pointLightEntitys[i] = InnoMath::createEntityID();

		m_pointLightTransformComponents[i]->m_parentEntity = m_pointLightEntitys[i];
		m_pointLightComponents[i]->m_parentEntity = m_pointLightEntitys[i];
		m_pointLightVisibleComponents[i]->m_parentEntity = m_pointLightEntitys[i];

		InnoGameSystem::addTransformComponent(m_pointLightTransformComponents[i]);
		InnoGameSystem::addLightComponent(m_pointLightComponents[i]);
		InnoGameSystem::addVisibleComponent(m_pointLightVisibleComponents[i]);
	}
	for (auto i = (unsigned int)0; i < pointLightMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < pointLightMatrixDim; j++)
		{
			m_pointLightTransformComponents[i * pointLightMatrixDim + j]->m_transformVector.setLocalPos(vec4((-(pointLightMatrixDim - 1.0) * pointLightBreadthInterval / 2.0) + (i * pointLightBreadthInterval), 2.0 + (j * pointLightBreadthInterval), 4.0, 1.0));
		}
	}
}

void InnocenceGarden::updateLights(double seed)
{
	m_directionalLightTransformComponent->m_transformVector.rotateInLocal(vec4(1.0, 0.0, 0.0, 0.0), 0.2);
	for (auto i = (unsigned int)0; i < m_pointLightComponents.size(); i += 4)
	{
		m_pointLightVisibleComponents[i]->m_albedo = vec4((sin(seed + i) + 1.0) * 5.0 / 2.0, 0.2 * 5.0, 0.4 * 5.0, 1.0);
		m_pointLightComponents[i]->m_color = vec4((sin(seed + i) + 1.0) * 5.0 / 2.0, 0.2 * 5.0, 0.4 * 5.0, 1.0);
		m_pointLightVisibleComponents[i + 1]->m_albedo = vec4(0.2 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 0.4 * 5.0, 1.0);
		m_pointLightComponents[i + 1]->m_color = vec4(0.2 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 0.4 * 5.0, 1.0);
		m_pointLightVisibleComponents[i + 2]->m_albedo = vec4(0.2 * 5.0, 0.4 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 1.0);
		m_pointLightComponents[i + 2]->m_color = vec4(0.2 * 5.0, 0.4 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 1.0);
		m_pointLightVisibleComponents[i + 3]->m_albedo = vec4((sin(seed + i * 2.0) + 1.0) * 5.0 / 2.0, (sin(seed + i * 3.0) + 1.0) * 5.0 / 2.0, (sin(seed + i * 5.0) + 1.0) * 5.0 / 2.0, 1.0);
		m_pointLightComponents[i + 3]->m_color = vec4((sin(seed + i * 2.0) + 1.0) * 5.0 / 2.0, (sin(seed + i * 3.0) + 1.0) * 5.0 / 2.0, (sin(seed + i * 5.0) + 1.0) * 5.0 / 2.0, 1.0);
	}
}

void InnocenceGarden::updateSpheres(double seed)
{
	m_pawnTransformComponent2->m_transformVector.rotateInGlobal(vec4(0.0, 1.0, 0.0, 0.0), 0.2);
}
