#include "InnocenceGarden.h"
#include "PlayerCharacter.h"

#include "../../engine/system/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

IGameInstance* g_pGameInstance;

namespace InnocenceGardenNS
{
	// root entity and its components
	EntityID m_rootEntity;
	TransformComponent* m_rootTransformComponent;

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

InnocenceGarden::InnocenceGarden(void)
{
	g_pGameInstance = this;
}

bool InnocenceGarden::setup()
{
	// setup root entity
	InnocenceGardenNS::m_rootTransformComponent = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
	InnocenceGardenNS::m_rootTransformComponent->m_parentTransformComponent = nullptr;

	InnocenceGardenNS::m_rootEntity = InnoMath::createEntityID();
	InnocenceGardenNS::m_rootTransformComponent->m_parentEntity = InnocenceGardenNS::m_rootEntity;

	// setup player character
	PlayerComponentCollection::m_transformComponent = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
	PlayerComponentCollection::m_transformComponent->m_parentTransformComponent = InnocenceGardenNS::m_rootTransformComponent;
	PlayerComponentCollection::m_visibleComponent = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>();
	PlayerComponentCollection::m_inputComponent = g_pCoreSystem->getGameSystem()->spawn<InputComponent>();
	PlayerComponentCollection::m_cameraComponent = g_pCoreSystem->getGameSystem()->spawn<CameraComponent>();

	PlayerComponentCollection::setup();
	PlayerComponentCollection::m_transformComponent->m_parentTransformComponent = InnocenceGardenNS::m_rootTransformComponent;
	PlayerComponentCollection::m_transformComponent->m_localTransformVector.m_pos = vec4(0.0f, 4.0f, 3.0f, 1.0f);

	PlayerComponentCollection::m_cameraComponent->m_drawFrustum = false;
	PlayerComponentCollection::m_cameraComponent->m_drawAABB = false;
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, button{ INNO_KEY_S, buttonStatus::PRESSED }, &PlayerComponentCollection::f_moveForward);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, button{ INNO_KEY_W, buttonStatus::PRESSED }, &PlayerComponentCollection::f_moveBackward);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, button{ INNO_KEY_A, buttonStatus::PRESSED }, &PlayerComponentCollection::f_moveLeft);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, button{ INNO_KEY_D, buttonStatus::PRESSED }, &PlayerComponentCollection::f_moveRight);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, button{ INNO_MOUSE_BUTTON_RIGHT, buttonStatus::PRESSED }, &PlayerComponentCollection::f_allowMove);
	g_pCoreSystem->getGameSystem()->registerButtonStatusCallback(PlayerComponentCollection::m_inputComponent, button{ INNO_MOUSE_BUTTON_RIGHT, buttonStatus::RELEASED }, &PlayerComponentCollection::f_forbidMove);
	g_pCoreSystem->getGameSystem()->registerMouseMovementCallback(PlayerComponentCollection::m_inputComponent, 0, &PlayerComponentCollection::f_rotateAroundPositiveYAxis);
	g_pCoreSystem->getGameSystem()->registerMouseMovementCallback(PlayerComponentCollection::m_inputComponent, 1, &PlayerComponentCollection::f_rotateAroundRightAxis);

	PlayerComponentCollection::m_parentEntity = InnoMath::createEntityID();

	//setup environment capture component
	InnocenceGardenNS::m_environmentCaptureComponent = g_pCoreSystem->getGameSystem()->spawn<EnvironmentCaptureComponent>();
	InnocenceGardenNS::m_environmentCaptureComponent->m_cubemapTextureFileName = "ibl//Playa_Sunrise.hdr";

	InnocenceGardenNS::m_EnvironmentCaptureEntity = InnoMath::createEntityID();

	InnocenceGardenNS::m_environmentCaptureComponent->m_parentEntity = InnocenceGardenNS::m_EnvironmentCaptureEntity;

	//setup directional light
	InnocenceGardenNS::m_directionalLightTransformComponent = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
	InnocenceGardenNS::m_directionalLightTransformComponent->m_parentTransformComponent = InnocenceGardenNS::m_rootTransformComponent;
	InnocenceGardenNS::m_directionalLightTransformComponent->m_localTransformVector.m_pos = vec4(0.0f, 4.0f, 0.0f, 1.0f);
	InnocenceGardenNS::m_directionalLightTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		InnocenceGardenNS::m_directionalLightTransformComponent->m_localTransformVector.m_rot,
		vec4(-1.0f, 0.0f, 0.0f, 0.0f),
		35.0f
	);
	InnocenceGardenNS::m_directionalLightComponent = g_pCoreSystem->getGameSystem()->spawn<LightComponent>();
	InnocenceGardenNS::m_directionalLightComponent->m_color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	InnocenceGardenNS::m_directionalLightComponent->m_lightType = lightType::DIRECTIONAL;
	InnocenceGardenNS::m_directionalLightComponent->m_drawAABB = false;

	InnocenceGardenNS::m_directionalLightEntity = InnoMath::createEntityID();

	InnocenceGardenNS::m_directionalLightTransformComponent->m_parentEntity = InnocenceGardenNS::m_directionalLightEntity;
	InnocenceGardenNS::m_directionalLightComponent->m_parentEntity = InnocenceGardenNS::m_directionalLightEntity;

	//setup landscape
	InnocenceGardenNS::m_landscapeTransformComponent = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
	InnocenceGardenNS::m_landscapeTransformComponent->m_parentTransformComponent = InnocenceGardenNS::m_rootTransformComponent;
	InnocenceGardenNS::m_landscapeTransformComponent->m_localTransformVector.m_scale = vec4(20.0f, 20.0f, 0.1f, 1.0f);
	InnocenceGardenNS::m_landscapeTransformComponent->m_localTransformVector.m_rot = InnoMath::rotateInLocal(
		InnocenceGardenNS::m_directionalLightTransformComponent->m_localTransformVector.m_rot,
		vec4(1.0f, 0.0f, 0.0f, 0.0f),
		90.0f
	);
	InnocenceGardenNS::m_landscapeVisibleComponent = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>();
	InnocenceGardenNS::m_landscapeVisibleComponent->m_visiblilityType = visiblilityType::STATIC_MESH;
	InnocenceGardenNS::m_landscapeVisibleComponent->m_meshShapeType = meshShapeType::CUBE;

	InnocenceGardenNS::m_landscapeEntity = InnoMath::createEntityID();

	InnocenceGardenNS::m_landscapeTransformComponent->m_parentEntity = InnocenceGardenNS::m_landscapeEntity;
	InnocenceGardenNS::m_landscapeVisibleComponent->m_parentEntity = InnocenceGardenNS::m_landscapeEntity;

	InnocenceGardenNS::setupLights();
	InnocenceGardenNS::setupSpheres();

	//setup pawn 1
	InnocenceGardenNS::m_pawnTransformComponent1 = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
	InnocenceGardenNS::m_pawnTransformComponent1->m_parentTransformComponent = InnocenceGardenNS::m_rootTransformComponent;
	InnocenceGardenNS::m_pawnTransformComponent1->m_localTransformVector.m_scale = vec4(0.1f, 0.1f, 0.1f, 1.0f);
	InnocenceGardenNS::m_pawnVisibleComponent1 = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>();
	InnocenceGardenNS::m_pawnVisibleComponent1->m_visiblilityType = visiblilityType::STATIC_MESH;
	InnocenceGardenNS::m_pawnVisibleComponent1->m_meshShapeType = meshShapeType::CUSTOM;
	InnocenceGardenNS::m_pawnVisibleComponent1->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	//m_pawnVisibleComponent1->m_modelFileName = "sponza//sponza.obj";
	//m_pawnVisibleComponent1->m_modelFileName = "cat//cat.obj";
	InnocenceGardenNS::m_pawnVisibleComponent1->m_textureWrapMethod = textureWrapMethod::REPEAT;
	InnocenceGardenNS::m_pawnVisibleComponent1->m_drawAABB = false;
	InnocenceGardenNS::m_pawnVisibleComponent1->m_useTexture = true;
	InnocenceGardenNS::m_pawnVisibleComponent1->m_albedo = vec4(0.95f, 0.93f, 0.88f, 1.0f);
	InnocenceGardenNS::m_pawnVisibleComponent1->m_MRA = vec4(0.0f, 0.35f, 1.0f, 1.0f);

	InnocenceGardenNS::m_pawnEntity1 = InnoMath::createEntityID();

	InnocenceGardenNS::m_pawnTransformComponent1->m_parentEntity = InnocenceGardenNS::m_pawnEntity1;
	InnocenceGardenNS::m_pawnVisibleComponent1->m_parentEntity = InnocenceGardenNS::m_pawnEntity1;

	//setup pawn 2
	InnocenceGardenNS::m_pawnTransformComponent2 = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
	InnocenceGardenNS::m_pawnTransformComponent2->m_parentTransformComponent = InnocenceGardenNS::m_rootTransformComponent;
	InnocenceGardenNS::m_pawnTransformComponent2->m_localTransformVector.m_scale = vec4(0.01f, 0.01f, 0.01f, 1.0f);
	InnocenceGardenNS::m_pawnTransformComponent2->m_localTransformVector.m_pos = vec4(0.0f, 0.2f, 3.5f, 1.0f);
	InnocenceGardenNS::m_pawnVisibleComponent2 = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>();
	InnocenceGardenNS::m_pawnVisibleComponent2->m_visiblilityType = visiblilityType::STATIC_MESH;
	InnocenceGardenNS::m_pawnVisibleComponent2->m_meshShapeType = meshShapeType::CUSTOM;
	InnocenceGardenNS::m_pawnVisibleComponent2->m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	InnocenceGardenNS::m_pawnVisibleComponent2->m_drawAABB = true;
	InnocenceGardenNS::m_pawnVisibleComponent2->m_modelFileName = "lantern//lantern.obj";

	InnocenceGardenNS::m_pawnEntity2 = InnoMath::createEntityID();

	InnocenceGardenNS::m_pawnTransformComponent2->m_parentEntity = InnocenceGardenNS::m_pawnEntity2;
	InnocenceGardenNS::m_pawnVisibleComponent2->m_parentEntity = InnocenceGardenNS::m_pawnEntity2;

	InnocenceGardenNS::m_objectStatus = objectStatus::ALIVE;
	return true;
}

bool InnocenceGarden::initialize()
{
	return true;
}

bool InnocenceGarden::update()
{
	InnocenceGardenNS::temp += 0.02f;
	InnocenceGardenNS::updateLights(InnocenceGardenNS::temp);
	InnocenceGardenNS::updateSpheres(InnocenceGardenNS::temp);
	return true;
}

bool InnocenceGarden::terminate()
{
	InnocenceGardenNS::m_objectStatus = objectStatus::SHUTDOWN;
	return true;
}

std::string InnocenceGarden::getGameName()
{
	return std::string("InnocenceGarden");
}

void InnocenceGardenNS::setupSpheres()
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
		m_sphereTransformComponents[i] = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
		m_sphereTransformComponents[i]->m_parentTransformComponent = m_rootTransformComponent;
		m_sphereTransformComponents[i]->m_localTransformVector.m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		m_sphereVisibleComponents[i] = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>();
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

void InnocenceGardenNS::setupLights()
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
		m_pointLightTransformComponents[i] = g_pCoreSystem->getGameSystem()->spawn<TransformComponent>();
		m_pointLightTransformComponents[i]->m_parentTransformComponent = m_rootTransformComponent;
		m_pointLightTransformComponents[i]->m_localTransformVector.m_scale = vec4(0.1f, 0.1f, 0.1f, 1.0f);
		m_pointLightComponents[i] = g_pCoreSystem->getGameSystem()->spawn<LightComponent>();
		m_pointLightVisibleComponents[i] = g_pCoreSystem->getGameSystem()->spawn<VisibleComponent>();
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

void InnocenceGardenNS::updateLights(float seed)
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

void InnocenceGardenNS::updateSpheres(float seed)
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
