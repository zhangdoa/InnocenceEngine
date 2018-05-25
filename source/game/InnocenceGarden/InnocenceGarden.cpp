#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}

InnocenceGarden::~InnocenceGarden()
{
}

void InnocenceGarden::setup()
{
	// setup root entity
	m_rootTransformComponent.m_parentTransform = nullptr;
	m_transformComponents.emplace_back(&m_rootTransformComponent);
	m_rootEntity.addChildComponent(&m_rootTransformComponent);

	// setup player character
	m_playCharacter.setup();
	m_playCharacter.getTransformComponent().m_parentTransform = &m_rootTransformComponent.m_transform;
	m_playCharacter.getTransformComponent().m_transform.setLocalPos(vec4(2.0, 1.0, 2.0, 1.0));
	//m_playCharacter.getTransformComponent().m_transform.rotateInLocal(vec4(0.0, 1.0, 0.0, 0.0), 45.0);
	m_playCharacter.getCameraComponent().m_drawFrustum = false;
	m_playCharacter.getCameraComponent().m_drawAABB = false;

	m_transformComponents.emplace_back(&m_playCharacter.getTransformComponent());
	m_cameraComponents.emplace_back(&m_playCharacter.getCameraComponent());
	m_inputComponents.emplace_back(&m_playCharacter.getInputComponent());

	//setup skybox
	m_skyboxTransformComponent.m_parentTransform = &m_rootTransformComponent.m_transform;
	m_skyboxVisibleComponent.m_visiblilityType = visiblilityType::SKYBOX;
	m_skyboxVisibleComponent.m_meshType = meshShapeType::CUBE;
	m_skyboxVisibleComponent.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	m_skyboxVisibleComponent.m_textureFileNameMap.emplace(textureFileNamePair(textureType::EQUIRETANGULAR, "ibl/Mono_Lake_B_Ref.hdr"));

	m_skyboxEntity.addChildComponent(&m_skyboxTransformComponent);
	m_skyboxEntity.addChildComponent(&m_skyboxVisibleComponent);

	m_transformComponents.emplace_back(&m_skyboxTransformComponent);
	m_visibleComponents.emplace_back(&m_skyboxVisibleComponent);

	//setup directional light
	m_directionalLightTransformComponent.m_parentTransform = &m_rootTransformComponent.m_transform;
	m_directionalLightTransformComponent.m_transform.setLocalPos(vec4(0.0, 4.0, 0.0, 1.0));
	m_directionalLightTransformComponent.m_transform.rotateInLocal(vec4(1.0, 0.0, 0.0, 0.0), -75.0);
	m_directionalLightTransformComponent.m_transform.rotateInLocal(vec4(0.0, 1.0, 0.0, 0.0), -35.0);
	m_directionalLightComponent.setColor(vec4(0.5, 0.3, 0.0, 1.0));
	m_directionalLightComponent.setlightType(lightType::DIRECTIONAL);
	m_directionalLightComponent.m_drawAABB = false;
	m_directionalLightVisibleComponent.m_visiblilityType = visiblilityType::BILLBOARD;
	m_directionalLightVisibleComponent.m_meshType = meshShapeType::CUBE;
	m_directionalLightVisibleComponent.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "lightbulb.png"));
	m_directionalLightVisibleComponent.m_albedo = vec4(0.5, 0.3, 0.0, 1.0);

	m_directionalLightEntity.addChildComponent(&m_directionalLightTransformComponent);
	m_directionalLightEntity.addChildComponent(&m_directionalLightComponent);
	m_directionalLightEntity.addChildComponent(&m_directionalLightVisibleComponent);

	m_transformComponents.emplace_back(&m_directionalLightTransformComponent);
	m_lightComponents.emplace_back(&m_directionalLightComponent);
	m_visibleComponents.emplace_back(&m_directionalLightVisibleComponent);

	//setup landscape
	m_landscapeTransformComponent.m_parentTransform = &m_rootTransformComponent.m_transform;
	m_landscapeTransformComponent.m_transform.setLocalScale(vec4(20.0, 20.0, 0.1, 1.0));
	m_landscapeTransformComponent.m_transform.rotateInLocal(vec4(1.0, 0.0, 0.0, 0.0), 90.0);
	m_landscapeVisibleComponent.m_visiblilityType = visiblilityType::STATIC_MESH;
	m_landscapeVisibleComponent.m_meshType = meshShapeType::CUBE;

	m_landscapeEntity.addChildComponent(&m_landscapeTransformComponent);
	m_landscapeEntity.addChildComponent(&m_landscapeVisibleComponent);

	m_transformComponents.emplace_back(&m_landscapeTransformComponent);
	m_visibleComponents.emplace_back(&m_landscapeVisibleComponent);

	//setup pawn 1
	m_pawnTransformComponent1.m_parentTransform = &m_rootTransformComponent.m_transform;
	m_pawnTransformComponent1.m_transform.setLocalScale(vec4(0.1, 0.1, 0.1, 1.0));
	m_pawnVisibleComponent1.m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnVisibleComponent1.m_meshType = meshShapeType::CUSTOM;
	//m_pawnVisibleComponent1.m_modelFileName = "sponza/sponza.obj";
	//m_pawnVisibleComponent1.m_modelFileName = "cat/cat.obj";
	m_pawnVisibleComponent1.m_textureWrapMethod = textureWrapMethod::REPEAT;
	m_pawnVisibleComponent1.m_drawAABB = false;
	m_pawnVisibleComponent1.m_useTexture = false;
	m_pawnVisibleComponent1.m_albedo = vec4(0.95, 0.93, 0.88, 1.0);
	m_pawnVisibleComponent1.m_MRA = vec4(0.0, 0.35, 1.0, 1.0);

	m_pawnEntity1.addChildComponent(&m_pawnTransformComponent1);
	m_pawnEntity1.addChildComponent(&m_pawnVisibleComponent1);

	m_transformComponents.emplace_back(&m_pawnTransformComponent1);
	m_visibleComponents.emplace_back(&m_pawnVisibleComponent1);

	//setup pawn 2
	m_pawnTransformComponent2.m_parentTransform = &m_rootTransformComponent.m_transform;
	m_pawnTransformComponent2.m_transform.setLocalScale(vec4(0.01, 0.01, 0.01, 1.0));
	m_pawnTransformComponent2.m_transform.setLocalPos(vec4(0.0, 0.2, 3.5, 1.0));
	m_pawnVisibleComponent2.m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnVisibleComponent2.m_meshType = meshShapeType::CUSTOM;
	m_pawnVisibleComponent2.m_drawAABB = true;
	m_pawnVisibleComponent2.m_modelFileName = "lantern/lantern.obj";
	m_pawnVisibleComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "lantern/lantern_Normal_OpenGL.jpg"));
	m_pawnVisibleComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "lantern/lantern_Base_Color.jpg"));
	m_pawnVisibleComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "lantern/lantern_Metallic.jpg"));
	m_pawnVisibleComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "lantern/lantern_Roughness.jpg"));
	m_pawnVisibleComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "lantern/lantern_Mixed_AO.jpg"));

	m_pawnEntity2.addChildComponent(&m_pawnTransformComponent2);
	m_pawnEntity2.addChildComponent(&m_pawnVisibleComponent2);

	m_transformComponents.emplace_back(&m_pawnTransformComponent2);
	m_visibleComponents.emplace_back(&m_pawnVisibleComponent2);

	setupLights();
	setupSpheres();

	m_rootEntity.setup();
}

void InnocenceGarden::initialize()
{	
	m_rootEntity.initialize();
}

void InnocenceGarden::update()
{
	temp += 0.02;
	updateLights(temp);
	updateSpheres(temp);
}

void InnocenceGarden::shutdown()
{	
	m_rootEntity.shutdown();
}

const objectStatus & InnocenceGarden::getStatus() const
{
	return m_objectStatus;
}


std::string InnocenceGarden::getGameName() const
{
	return std::string{ typeid(*this).name() }.substr(std::string{ typeid(*this).name() }.find("class"), std::string::npos);
}

std::vector<TransformComponent*>& InnocenceGarden::getTransformComponents()
{
	return m_transformComponents;
}

std::vector<CameraComponent*>& InnocenceGarden::getCameraComponents()
{
	return m_cameraComponents;
}

std::vector<InputComponent*>& InnocenceGarden::getInputComponents()
{
	return m_inputComponents;
}

std::vector<LightComponent*>& InnocenceGarden::getLightComponents()
{
	return m_lightComponents;
}

std::vector<VisibleComponent*>& InnocenceGarden::getVisibleComponents()
{
	return m_visibleComponents;
}

void InnocenceGarden::setupSpheres()
{
	unsigned int sphereMatrixDim = 8;
	double sphereBreadthInterval = 4.0;
	for (auto i = (unsigned int)0; i < sphereMatrixDim * sphereMatrixDim; i++)
	{
		m_sphereTransformComponents.emplace_back();
		m_sphereVisibleComponents.emplace_back();
		m_sphereEntitys.emplace_back();
	}
	for (auto i = (unsigned int)0; i < m_sphereVisibleComponents.size(); i++)
	{
		m_sphereTransformComponents[i].m_parentTransform = &m_rootTransformComponent.m_transform;
		m_sphereTransformComponents[i].m_transform.setLocalScale(vec4(10.0, 10.0, 10.0, 1.0));
		m_sphereVisibleComponents[i].m_visiblilityType = visiblilityType::STATIC_MESH;
		m_sphereVisibleComponents[i].m_meshType = meshShapeType::CUSTOM;
		m_sphereVisibleComponents[i].m_drawAABB = true;
		m_sphereVisibleComponents[i].m_modelFileName = "lteOrb/lteOrb.obj";
		m_sphereVisibleComponents[i].m_useTexture = false;

		m_sphereEntitys[i].addChildComponent(&m_sphereTransformComponents[i]);
		m_sphereEntitys[i].addChildComponent(&m_sphereVisibleComponents[i]);

		m_transformComponents.emplace_back(&m_sphereTransformComponents[i]);
		m_visibleComponents.emplace_back(&m_sphereVisibleComponents[i]);
	}
	for (auto i = (unsigned int)0; i < m_sphereVisibleComponents.size(); i += 4)
	{
		//Copper
		m_sphereVisibleComponents[i].m_albedo = vec4(0.95, 0.64, 0.54 ,1.0);
		//m_sphereVisibleComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/rustediron2_normal.png"));
		//m_sphereVisibleComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/rustediron2_basecolor.png"));
		//m_sphereVisibleComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/rustediron2_metallic.png"));
		//m_sphereVisibleComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/rustediron2_roughness.png"));
		//Gold
		m_sphereVisibleComponents[i + 1].m_albedo = vec4(1.00, 0.71, 0.29, 1.0);
		//m_sphereVisibleComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/bamboo-wood-semigloss-normal.png"));
		//m_sphereVisibleComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/bamboo-wood-semigloss-albedo.png"));
		//m_sphereVisibleComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/bamboo-wood-semigloss-metal.png"));
		//m_sphereVisibleComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "PBS/bamboo-wood-semigloss-ao.png"));
		//Iron
		m_sphereVisibleComponents[i + 2].m_albedo = vec4(0.56, 0.57, 0.58, 1.0);
		//m_sphereVisibleComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/greasy-metal-pan1-normal.png"));
		//m_sphereVisibleComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/greasy-metal-pan1-albedo.png"));
		//m_sphereVisibleComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/greasy-metal-pan1-metal.png"));
		//m_sphereVisibleComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/greasy-metal-pan1-roughness.png"));
		//Silver
		m_sphereVisibleComponents[i + 3].m_albedo = vec4(0.95, 0.93, 0.88, 1.0);
		//m_sphereVisibleComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/roughrock1-normal.png"));
		//m_sphereVisibleComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/roughrock1-albedo.png"));
		//m_sphereVisibleComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/roughrock1-metalness.png"));
		//m_sphereVisibleComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/roughrock1-roughness.png"));
		//m_sphereVisibleComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "PBS/roughrock1-ao.png"));
	}
	for (auto i = (unsigned int)0; i < sphereMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < sphereMatrixDim; j++)
		{
			m_sphereTransformComponents[i * sphereMatrixDim + j].m_transform.setLocalPos(vec4((-(sphereMatrixDim - 1.0) * sphereBreadthInterval / 2.0) + (i * sphereBreadthInterval), 0.0, (j * sphereBreadthInterval) - 2.0 * (sphereMatrixDim - 1), 1.0));
			m_sphereVisibleComponents[i * sphereMatrixDim + j].m_MRA = vec4((double)(i) / (double)(sphereMatrixDim), (double)(j) / (double)(sphereMatrixDim), 1.0, 1.0);
		}
	}	
}

void InnocenceGarden::setupLights()
{
	unsigned int pointLightMatrixDim = 8;
	double pointLightBreadthInterval = 4.0;
	for (auto i = (unsigned int)0; i < pointLightMatrixDim * pointLightMatrixDim; i++)
	{
		m_pointLightTransformComponents.emplace_back();
		m_pointLightComponents.emplace_back();
		m_pointLightVisibleComponents.emplace_back();
		m_pointLightEntitys.emplace_back();
	}
	for (auto i = (unsigned int)0; i < m_pointLightComponents.size(); i++)
	{
		m_pointLightTransformComponents[i].m_parentTransform = &m_rootTransformComponent.m_transform;
		m_pointLightTransformComponents[i].m_transform.setLocalScale(vec4(0.1, 0.1, 0.1, 1.0));
		m_pointLightVisibleComponents[i].m_visiblilityType = visiblilityType::EMISSIVE;
		m_pointLightVisibleComponents[i].m_meshType = meshShapeType::SPHERE;
		m_pointLightVisibleComponents[i].m_useTexture = false;

		m_pointLightEntitys[i].addChildComponent(&m_pointLightTransformComponents[i]);
		m_pointLightEntitys[i].addChildComponent(&m_pointLightComponents[i]);
		m_pointLightEntitys[i].addChildComponent(&m_pointLightVisibleComponents[i]);

		m_transformComponents.emplace_back(&m_pointLightTransformComponents[i]);
		m_lightComponents.emplace_back(&m_pointLightComponents[i]);
		m_visibleComponents.emplace_back(&m_pointLightVisibleComponents[i]);
	}
	for (auto i = (unsigned int)0; i < pointLightMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < pointLightMatrixDim; j++)
		{
			m_pointLightTransformComponents[i * pointLightMatrixDim + j].m_transform.setLocalPos(vec4((-(pointLightMatrixDim - 1.0) * pointLightBreadthInterval / 2.0) + (i * pointLightBreadthInterval), 2.0 + (j * pointLightBreadthInterval), 4.0, 1.0));
		}
	}
}

void InnocenceGarden::updateLights(double seed)
{
	for (auto i = (unsigned int)0; i < m_pointLightComponents.size(); i+=4)
	{
		m_pointLightVisibleComponents[i].m_albedo = vec4((sin(seed + i) + 1.0) * 5.0 / 2.0, 0.2 * 5.0, 0.4 * 5.0, 1.0);
		m_pointLightComponents[i].setColor(vec4((sin(seed + i) + 1.0) * 5.0 / 2.0, 0.2 * 5.0, 0.4 * 5.0, 1.0));
		m_pointLightVisibleComponents[i + 1].m_albedo = vec4(0.2 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 0.4 * 5.0, 1.0);
		m_pointLightComponents[i + 1].setColor(vec4(0.2 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 0.4 * 5.0, 1.0));
		m_pointLightVisibleComponents[i + 2].m_albedo = vec4(0.2 * 5.0, 0.4 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 1.0);
		m_pointLightComponents[i + 2].setColor(vec4(0.2 * 5.0, 0.4 * 5.0, (sin(seed + i) + 1.0) * 5.0 / 2.0, 1.0));
		m_pointLightVisibleComponents[i + 3].m_albedo = vec4((sin(seed + i * 2.0) + 1.0) * 5.0 / 2.0, (sin(seed + i* 3.0) + 1.0) * 5.0 / 2.0, (sin(seed + i * 5.0) + 1.0) * 5.0 / 2.0, 1.0);
		m_pointLightComponents[i + 3].setColor(vec4((sin(seed + i * 2.0 ) + 1.0) * 5.0 / 2.0, (sin(seed + i* 3.0) + 1.0) * 5.0 / 2.0, (sin(seed + i * 5.0) + 1.0) * 5.0 / 2.0, 1.0));
	}
}

void InnocenceGarden::updateSpheres(double seed)
{
}
