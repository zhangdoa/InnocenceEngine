#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}

InnocenceGarden::~InnocenceGarden()
{
}

void InnocenceGarden::setup()
{
	m_rootEntity.addChildEntity(&m_playCharacter);

	m_rootEntity.addChildEntity(&m_skyboxEntity);

	m_rootEntity.addChildEntity(&m_directionalLightEntity);

	m_rootEntity.addChildEntity(&m_landscapeEntity);
	m_rootEntity.addChildEntity(&m_pawnEntity1);
	m_rootEntity.addChildEntity(&m_pawnEntity2);

	m_playCharacter.getTransform()->setPos(vec4(1.0, 2.0, 3.0, 1.0));
	m_playCharacter.getTransform()->rotate(vec4(0.0, 1.0, 0.0, 0.0), 60.0);
	m_playCharacter.getCameraComponent().m_drawFrustum = false;
	m_playCharacter.getCameraComponent().m_drawAABB = false;
	m_cameraComponents.emplace_back(&m_playCharacter.getCameraComponent());
	m_inputComponents.emplace_back(&m_playCharacter.getInputComponent());

	m_skyboxComponent.m_visiblilityType = visiblilityType::SKYBOX;
	m_skyboxComponent.m_meshType = meshShapeType::CUBE;
	m_skyboxComponent.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	m_skyboxComponent.m_textureFileNameMap.emplace(textureFileNamePair(textureType::EQUIRETANGULAR, "ibl/GCanyon_C_YumaPoint_3k.hdr"));
	m_skyboxEntity.addChildComponent(&m_skyboxComponent);
	m_visibleComponents.emplace_back(&m_skyboxComponent);

	m_directionalLightComponent.setColor(vec4(0.5, 0.3, 0.0, 1.0));
	m_directionalLightComponent.setlightType(lightType::DIRECTIONAL);
	m_directionalLightComponent.m_drawAABB = false;
	m_directionalLightEntity.addChildComponent(&m_directionalLightComponent);
	m_directionalLightEntity.getTransform()->setPos(vec4(-2.0, 4.0, -1.0, 1.0));
	m_directionalLightEntity.getTransform()->rotate(vec4(1.0, 0.0, 0.0, 0.0), 45);
	m_lightComponents.emplace_back(&m_directionalLightComponent);

	m_directionalLightBillboardComponent.m_visiblilityType = visiblilityType::BILLBOARD;
	m_directionalLightBillboardComponent.m_meshType = meshShapeType::CUBE;
	m_directionalLightBillboardComponent.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "lightbulb.png"));
	m_directionalLightEntity.addChildComponent(&m_directionalLightBillboardComponent);
	m_visibleComponents.emplace_back(&m_directionalLightBillboardComponent);

	m_landscapeComponent.m_visiblilityType = visiblilityType::STATIC_MESH;
	m_landscapeComponent.m_meshType = meshShapeType::CUBE;
	m_landscapeEntity.addChildComponent(&m_landscapeComponent);
	m_landscapeEntity.getTransform()->setScale(vec4(20.0, 20.0, 0.1, 1.0));
	m_landscapeEntity.getTransform()->rotate(vec4(1.0, 0.0, 0.0, 0.0), 90.0);
	m_visibleComponents.emplace_back(&m_landscapeComponent);

	m_pawnComponent1.m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnComponent1.m_meshType = meshShapeType::CUSTOM;
	//m_pawnComponent1.m_modelFileName = "sponza/sponza.obj";
	m_pawnComponent1.m_textureWrapMethod = textureWrapMethod::REPEAT;
	m_pawnComponent1.m_drawAABB = true;
	m_pawnComponent1.m_useTexture = true;
	m_pawnComponent1.m_albedo = vec4(0.95, 0.93, 0.88, 1.0);
	m_pawnComponent1.m_MRA = vec4(0.0, 0.35, 1.0, 1.0);
	m_pawnEntity1.addChildComponent(&m_pawnComponent1);
	m_pawnEntity1.getTransform()->rotate(vec4(0.0, 1.0, 0.0, 0.0), 90.0);
	m_pawnEntity1.getTransform()->setScale(vec4(0.1, 0.1, 0.1, 1.0));
	m_visibleComponents.emplace_back(&m_pawnComponent1);

	m_pawnComponent2.m_visiblilityType = visiblilityType::STATIC_MESH;
	m_pawnComponent2.m_meshType = meshShapeType::CUSTOM;
	m_pawnComponent2.m_drawAABB = true;
	m_pawnComponent2.m_modelFileName = "lantern/lantern.obj";
	m_pawnComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "lantern/lantern_Normal_OpenGL.jpg"));
	m_pawnComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "lantern/lantern_Base_Color.jpg"));
	m_pawnComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "lantern/lantern_Metallic.jpg"));
	m_pawnComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "lantern/lantern_Roughness.jpg"));
	m_pawnComponent2.m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "lantern/lantern_Mixed_AO.jpg"));
	m_pawnEntity2.addChildComponent(&m_pawnComponent2);
	m_pawnEntity2.getTransform()->setScale(vec4(0.01, 0.01, 0.01, 1.0));
	m_pawnEntity2.getTransform()->setPos(vec4(0.0, 0.2, 3.5, 1.0));
	m_visibleComponents.emplace_back(&m_pawnComponent2);

//	setupLights();
	setupSpheres();

	m_rootEntity.setup();
}

void InnocenceGarden::initialize()
{	
	m_rootEntity.initialize();
}

void InnocenceGarden::update()
{
	//for (size_t i = 0; i < 1000000; i++)
	//{
	//	auto a = mat4();
	//	a.initializeToPerspectiveMatrix((70.0 / 180.0) * PI, (16.0 / 9.0), 0.1, 1000000.0);
	//	a = a * a;
	//}
	//mat4 t;
	//t.initializeToPerspectiveMatrix((70.0 / 180.0) * PI, (16.0 / 9.0), 0.1, 1000000.0);
	//g_pLogSystem->printLog("Original: ");
	//g_pLogSystem->printLog(t);
	//g_pLogSystem->printLog("Determinant: ");
	//g_pLogSystem->printLog(t.getDeterminant());
	//g_pLogSystem->printLog("Transpose: ");
	//g_pLogSystem->printLog(t.transpose());
	//g_pLogSystem->printLog("Inverse: ");
	//g_pLogSystem->printLog(t.inverse());
	//g_pLogSystem->printLog("Inverse validation: ");
	//g_pLogSystem->printLog(t.inverse().mul(t));
	//auto v = vec4(0.0, 1.0, 0.0, 0.0).cross(vec4(0.0, 0.0, 1.0, 0.0));
	temp += 0.02f;
	updateLights(temp);
	updateSpheres(temp);
	m_rootEntity.update();
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
		m_sphereComponents.emplace_back();
		m_sphereEntitys.emplace_back();
	}
	for (auto i = (unsigned int)0; i < m_sphereComponents.size(); i++)
	{
		m_sphereComponents[i].m_visiblilityType = visiblilityType::STATIC_MESH;
		m_sphereComponents[i].m_meshType = meshShapeType::SPHERE;
		m_sphereComponents[i].m_drawAABB = true;
		m_sphereComponents[i].m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
		m_sphereComponents[i].m_useTexture = false;
		m_rootEntity.addChildEntity(&m_sphereEntitys[i]);
		m_sphereEntitys[i].addChildComponent(&m_sphereComponents[i]);
		m_visibleComponents.emplace_back(&m_sphereComponents[i]);
	}
	for (auto i = (unsigned int)0; i < m_sphereComponents.size(); i += 2)
	{
		////Copper
		m_sphereComponents[i].m_albedo = vec4(0.95, 0.93, 0.88 ,1.0);
		m_sphereComponents[i + 1].m_albedo = vec4(0.0, 0.0, 0.0, 1.0);
		//m_sphereComponents[i].m_albedo = vec4(0.95, 0.64, 0.54 ,1.0);
		//m_sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/rustediron2_normal.png"));
		//m_sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/rustediron2_basecolor.png"));
		//m_sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/rustediron2_metallic.png"));
		//m_sphereComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/rustediron2_roughness.png"));
		////Gold
		//m_sphereComponents[i + 1].m_albedo = vec4(1.00, 0.71, 0.29, 1.0);
		//m_sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/bamboo-wood-semigloss-normal.png"));
		//m_sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/bamboo-wood-semigloss-albedo.png"));
		//m_sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/bamboo-wood-semigloss-metal.png"));
		//m_sphereComponents[i + 1].m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "PBS/bamboo-wood-semigloss-ao.png"));
		////Iron
		//m_sphereComponents[i + 2].m_albedo = vec4(0.56, 0.57, 0.58, 1.0);
		//m_sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/greasy-metal-pan1-normal.png"));
		//m_sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/greasy-metal-pan1-albedo.png"));
		//m_sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/greasy-metal-pan1-metal.png"));
		//m_sphereComponents[i + 2].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/greasy-metal-pan1-roughness.png"));
		////Silver
		//m_sphereComponents[i + 3].m_albedo = vec4(0.95, 0.93, 0.88, 1.0);
		//m_sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::NORMAL, "PBS/roughrock1-normal.png"));
		//m_sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "PBS/roughrock1-albedo.png"));
		//m_sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::METALLIC, "PBS/roughrock1-metalness.png"));
		//m_sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ROUGHNESS, "PBS/roughrock1-roughness.png"));
		//m_sphereComponents[i + 3].m_textureFileNameMap.emplace(textureFileNamePair(textureType::AMBIENT_OCCLUSION, "PBS/roughrock1-ao.png"));
	}
	for (auto i = (unsigned int)0; i < sphereMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < sphereMatrixDim; j++)
		{
			m_sphereEntitys[i * sphereMatrixDim + j].getTransform()->setPos(vec4((-(sphereMatrixDim - 1.0) * sphereBreadthInterval / 2.0) + (i * sphereBreadthInterval), 2.0, (j * sphereBreadthInterval) - 2.0 * (sphereMatrixDim - 1), 1.0));
			m_sphereComponents[i * sphereMatrixDim + j].m_MRA = vec4((double)(i) / (double)(sphereMatrixDim), (double)(j) / (double)(sphereMatrixDim), 1.0, 1.0);
		}
	}	
}

void InnocenceGarden::setupLights()
{
	unsigned int pointLightMatrixDim = 8;
	double pointLightBreadthInterval = 4.0;
	for (auto i = (unsigned int)0; i < pointLightMatrixDim * pointLightMatrixDim; i++)
	{
		m_pointLightComponents.emplace_back();
		m_pointLightBillboardComponents.emplace_back();
		m_pointLightEntitys.emplace_back();

	}
	for (auto i = (unsigned int)0; i < m_pointLightComponents.size(); i++)
	{
		m_lightComponents.emplace_back(&m_pointLightComponents[i]);

		m_pointLightBillboardComponents[i].m_visiblilityType = visiblilityType::EMISSIVE;
		m_pointLightBillboardComponents[i].m_meshType = meshShapeType::SPHERE;
		//m_pointLightBillboardComponents[i].m_textureFileNameMap.emplace(textureFileNamePair(textureType::ALBEDO, "lightbulb.png"));
		m_visibleComponents.emplace_back(&m_pointLightBillboardComponents[i]);

		m_rootEntity.addChildEntity(&m_pointLightEntitys[i]);
		m_pointLightEntitys[i].addChildComponent(&m_pointLightComponents[i]);
		m_pointLightEntitys[i].addChildComponent(&m_pointLightBillboardComponents[i]);
		m_pointLightEntitys[i].getTransform()->setScale(vec4(0.1, 0.1, 0.1, 1.0));
	}
	for (auto i = (unsigned int)0; i < pointLightMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < pointLightMatrixDim; j++)
		{
			m_pointLightEntitys[i * pointLightMatrixDim + j].getTransform()->setPos(vec4((-(pointLightMatrixDim - 1.0) * pointLightBreadthInterval / 2.0) + (i * pointLightBreadthInterval), 2.0 + (j * pointLightBreadthInterval), 4.0, 1.0));
		}
	}
}

void InnocenceGarden::updateLights(double seed)
{
	//m_directionalLightEntity.getTransform()->rotate(vec4(1.0, 0.0, 0.0, 0.0), 0.5);
	//m_directionalLightBillboardComponent.m_albedo = vec4((sin(seed) + 1.0) * 25.0 / 2.0, 0.2f * 25.0, 0.4f * 25.0, 1.0);
	//m_directionalLightComponent.setColor(vec4((sin(seed) + 1.0) * 25.0 / 2.0, 0.2f * 25.0, 0.4f * 25.0, 1.0));
	for (auto i = (unsigned int)0; i < m_pointLightComponents.size(); i+=4)
	{
		m_pointLightBillboardComponents[i].m_albedo = vec4((sin(seed + i) + 1.0) * 25.0 / 2.0, 0.2f * 25.0, 0.4f * 25.0, 1.0);
		m_pointLightComponents[i].setColor(vec4((sin(seed + i) + 1.0) * 25.0 / 2.0, 0.2f * 25.0, 0.4f * 25.0, 1.0));
		m_pointLightBillboardComponents[i + 1].m_albedo = vec4(0.2f * 25.0, (sin(seed + i) + 1.0) * 25.0 / 2.0, 0.4f * 25.0, 1.0);
		m_pointLightComponents[i + 1].setColor(vec4(0.2f * 25.0, (sin(seed + i) + 1.0) * 25.0 / 2.0, 0.4f * 25.0, 1.0));
		m_pointLightBillboardComponents[i + 2].m_albedo = vec4(0.2f * 25.0, 0.4f * 25.0, (sin(seed + i) + 1.0) * 25.0 / 2.0, 1.0);
		m_pointLightComponents[i + 2].setColor(vec4(0.2f * 25.0, 0.4f * 25.0, (sin(seed + i) + 1.0) * 25.0 / 2.0, 1.0));
		m_pointLightBillboardComponents[i + 3].m_albedo = vec4((sin(seed + i * 2.0) + 1.0) * 25.0 / 2.0, (sin(seed + i* 3.0) + 1.0) * 25.0 / 2.0, (sin(seed + i * 5.0) + 1.0) * 25.0 / 2.0, 1.0);
		m_pointLightComponents[i + 3].setColor(vec4((sin(seed + i * 2.0 ) + 1.0) * 25.0 / 2.0, (sin(seed + i* 3.0) + 1.0) * 25.0 / 2.0, (sin(seed + i * 5.0) + 1.0) * 25.0 / 2.0, 1.0));
	}
}

void InnocenceGarden::updateSpheres(double seed)
{
	m_pawnEntity2.getTransform()->rotate(vec4(0.0, 1.0, 0.0, 0.0), 0.5);
	for (auto i = (unsigned int)0; i < m_sphereEntitys.size(); i++)
	{
		m_sphereEntitys[i].getTransform()->rotate(vec4(0.0, 1.0, 0.0, 0.0), 0.01 * i);
		m_sphereEntitys[i].getTransform()->setPos(m_sphereEntitys[i].getTransform()->getPos() + vec4(cos(seed) * 0.01, 0.0, 0.0, 1.0));
	}
}
