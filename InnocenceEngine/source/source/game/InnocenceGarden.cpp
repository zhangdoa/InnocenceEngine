#include "../main/stdafx.h"
#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}

InnocenceGarden::~InnocenceGarden()
{
}

void InnocenceGarden::setup()
{
	rootActor.addChildActor(&playCharacter);

	rootActor.addChildActor(&skyboxActor);

	rootActor.addChildActor(&directionalLightActor);

	rootActor.addChildActor(&landscapeActor);
	rootActor.addChildActor(&pawnActor1);
	rootActor.addChildActor(&pawnActor2);

	SceneGraphManager::getInstance().addToCameraQueue(&playCharacter.getCameraComponent());

	SceneGraphManager::getInstance().addToInputQueue(&playCharacter.getInputComponent());

	skyboxComponent.m_visiblilityType = visiblilityType::SKYBOX;
	skyboxComponent.m_textureWrapMethod = textureWrapMethod::CLAMP_TO_EDGE;
	skyboxActor.addChildComponent(&skyboxComponent);

	directionalLightComponent.setColor(vec3(1.0f, 1.0f, 1.0f));
	directionalLightComponent.setlightType(lightType::DIRECTIONAL);
	directionalLightComponent.setDirection(vec3(0.0f, 0.0f, -0.85f));
	SceneGraphManager::getInstance().addToLightQueue(&directionalLightComponent);

	landscapeStaticMeshComponent.m_visiblilityType = visiblilityType::STATIC_MESH;
	landscapeActor.addChildComponent(&landscapeStaticMeshComponent);

	pawnMeshComponent1.m_visiblilityType = visiblilityType::STATIC_MESH;
	pawnActor1.addChildComponent(&pawnMeshComponent1);

	pawnMeshComponent2.m_visiblilityType = visiblilityType::STATIC_MESH;
	pawnActor2.addChildComponent(&pawnMeshComponent2);

	playCharacter.getTransform()->setPos(vec3(0.0f, 2.0f, 5.0f));
	landscapeActor.getTransform()->setScale(vec3(20.0f, 20.0f, 0.1f));
	landscapeActor.getTransform()->rotate(vec3(1.0f, 0.0f, 0.0f), 90.0f);

	landscapeActor.getTransform()->setPos(vec3(0.0f, 0.0f, 0.0f));
	pawnActor1.getTransform()->setScale(vec3(0.02f, 0.02f, 0.02f));
	pawnActor1.getTransform()->setPos(vec3(0.0f, 0.2f, -1.5f));
	pawnActor2.getTransform()->setScale(vec3(0.02f, 0.02f, 0.02f));
	pawnActor2.getTransform()->setPos(vec3(0.0f, 0.2f, 3.5f));

	setupLights();
	setupSpheres();

	rootActor.setup();
}

void InnocenceGarden::initialize()
{	
	initializeSpheres();

	AssetManager::getInstance().loadAsset("lantern/lantern.obj", pawnMeshComponent2);
	AssetManager::getInstance().loadAsset("lantern/lantern_Normal_OpenGL.jpg", textureType::NORMAL, pawnMeshComponent2);
	AssetManager::getInstance().loadAsset("lantern/lantern_Base_Color.jpg", textureType::ALBEDO, pawnMeshComponent2);
	AssetManager::getInstance().loadAsset("lantern/lantern_Metallic.jpg", textureType::METALLIC, pawnMeshComponent2);
	AssetManager::getInstance().loadAsset("lantern/lantern_Roughness.jpg", textureType::ROUGHNESS, pawnMeshComponent2);
	AssetManager::getInstance().loadAsset("lantern/lantern_Mixed_AO.jpg", textureType::AMBIENT_OCCLUSION, pawnMeshComponent2);

	AssetManager::getInstance().addUnitMesh(skyboxComponent, unitMeshType::CUBE);
	AssetManager::getInstance().loadAsset("ibl/Brooklyn_Bridge_Planks_2k.hdr", textureType::EQUIRETANGULAR, skyboxComponent);
	AssetManager::getInstance().addUnitMesh(landscapeStaticMeshComponent, unitMeshType::CUBE);

	

	rootActor.initialize();
}

void InnocenceGarden::update()
{
	temp += 0.02f;
	updateLights(temp);
	updateSpheres(temp);
	rootActor.update();
}

void InnocenceGarden::shutdown()
{	
	rootActor.shutdown();
}

void InnocenceGarden::setupSpheres()
{
	unsigned int sphereMatrixDim = 8;
	float sphereBreadthInterval = 4.0;
	for (auto i = (unsigned int)0; i < sphereMatrixDim * sphereMatrixDim; i++)
	{
		sphereActors.emplace_back();
		sphereComponents.emplace_back();
	}
	for (auto i = (unsigned int)0; i < sphereComponents.size(); i++)
	{
		sphereComponents[i].m_meshDrawMethod = meshDrawMethod::TRIANGLE_STRIP;
		sphereComponents[i].m_visiblilityType = visiblilityType::STATIC_MESH;
		sphereComponents[i].m_useTexture = false;
		rootActor.addChildActor(&sphereActors[i]);
		sphereActors[i].addChildComponent(&sphereComponents[i]);
	}

	for (auto i = (unsigned int)0; i < sphereMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < sphereMatrixDim; j++)
		{
			sphereActors[i * sphereMatrixDim + j].getTransform()->setPos(vec3((-(sphereMatrixDim - 1.0f) * sphereBreadthInterval / 2.0f) + (i * sphereBreadthInterval), 2.0f, (j * sphereBreadthInterval) - 2.0f * (sphereMatrixDim - 1)));
			sphereComponents[i * sphereMatrixDim + j].m_MRA = vec3((float)i / (float)sphereMatrixDim, (float)j / (float)sphereMatrixDim, 1.0);
		}
	}	
}

void InnocenceGarden::initializeSpheres()
{
	for (auto i = (unsigned int)0; i < sphereComponents.size(); i++)
	{
		AssetManager::getInstance().addUnitMesh(sphereComponents[i], unitMeshType::SPHERE);
	}
	for (auto i = (unsigned int)0; i < sphereComponents.size(); i+=4)
	{
		////Copper
		sphereComponents[i].m_albedo = vec3(0.95, 0.64, 0.54);
		//AssetManager::getInstance().loadAsset("PBS/rustediron2_basecolor.png", textureType::ALBEDO, sphereComponents[i]);
		//AssetManager::getInstance().loadAsset("PBS/rustediron2_metallic.png", textureType::METALLIC, sphereComponents[i]);
		//AssetManager::getInstance().loadAsset("PBS/rustediron2_normal.png", textureType::NORMAL, sphereComponents[i]);
		//AssetManager::getInstance().loadAsset("PBS/rustediron2_roughness.png", textureType::ROUGHNESS, sphereComponents[i]);
		////Gold
		sphereComponents[i + 1].m_albedo = vec3(1.00, 0.71, 0.29);
		//AssetManager::getInstance().loadAsset("PBS/bamboo-wood-semigloss-albedo.png", textureType::ALBEDO, sphereComponents[i + 1]);
		//AssetManager::getInstance().loadAsset("PBS/bamboo-wood-semigloss-metal.png", textureType::METALLIC, sphereComponents[i + 1]);
		//AssetManager::getInstance().loadAsset("PBS/bamboo-wood-semigloss-normal.png", textureType::NORMAL, sphereComponents[i + 1]);
		//AssetManager::getInstance().loadAsset("PBS/bamboo-wood-semigloss-ao.png", textureType::AMBIENT_OCCLUSION, sphereComponents[i + 1]);
		////Iron
		sphereComponents[i + 2].m_albedo = vec3(0.56, 0.57, 0.58);
		//AssetManager::getInstance().loadAsset("PBS/greasy-metal-pan1-albedo.png", textureType::ALBEDO, sphereComponents[i + 2]);
		//AssetManager::getInstance().loadAsset("PBS/greasy-metal-pan1-metal.png", textureType::METALLIC, sphereComponents[i + 2]);
		//AssetManager::getInstance().loadAsset("PBS/greasy-metal-pan1-normal.png", textureType::NORMAL, sphereComponents[i + 2]);
		//AssetManager::getInstance().loadAsset("PBS/greasy-metal-pan1-roughness.png", textureType::ROUGHNESS, sphereComponents[i + 2]);
		////Silver
		sphereComponents[i + 3].m_albedo = vec3(0.95, 0.93, 0.88);
		//AssetManager::getInstance().loadAsset("PBS/roughrock1-albedo.png", textureType::ALBEDO, sphereComponents[i + 3]);
		//AssetManager::getInstance().loadAsset("PBS/roughrock1-metalness.png", textureType::METALLIC, sphereComponents[i + 3]);
		//AssetManager::getInstance().loadAsset("PBS/roughrock1-normal.png", textureType::NORMAL, sphereComponents[i + 3]);
		//AssetManager::getInstance().loadAsset("PBS/roughrock1-roughness.png", textureType::ROUGHNESS, sphereComponents[i + 3]);
		//AssetManager::getInstance().loadAsset("PBS/roughrock1-ao.png", textureType::AMBIENT_OCCLUSION, sphereComponents[i + 3]);
	}
}

void InnocenceGarden::setupLights()
{
	unsigned int pointLightMatrixDim = 8;
	float pointLightBreadthInterval = 4.0;
	for (auto i = (unsigned int)0; i < pointLightMatrixDim * pointLightMatrixDim; i++)
	{
		pointLightActors.emplace_back();
		pointLightComponents.emplace_back();
	}
	for (auto i = (unsigned int)0; i < pointLightComponents.size(); i++)
	{
		rootActor.addChildActor(&pointLightActors[i]);
		pointLightActors[i].addChildComponent(&pointLightComponents[i]);

		SceneGraphManager::getInstance().addToLightQueue(&pointLightComponents[i]);
	}
	for (auto i = (unsigned int)0; i < pointLightMatrixDim; i++)
	{
		for (auto j = (unsigned int)0; j < pointLightMatrixDim; j++)
		{
			pointLightActors[i * pointLightMatrixDim + j].getTransform()->setPos(vec3((-(pointLightMatrixDim - 1.0) * pointLightBreadthInterval / 2.0) + (i * pointLightBreadthInterval), 2.0 + (j * pointLightBreadthInterval), 4.0));
		}
	}
}

void InnocenceGarden::updateLights(float seed)
{
	for (auto i = (unsigned int)0; i < pointLightComponents.size(); i+=4)
	{
		pointLightComponents[i].setColor(vec3((sin(seed + i) + 1.0f) * 10.0f / 2.0f, 0.2f * 10.0f, 0.4f * 10.0f));
		pointLightComponents[i + 1].setColor(vec3(0.2f * 10.0f, (sin(seed + i) + 1.0f) * 10.0f / 2.0f, 0.4f * 10.0f));
		pointLightComponents[i + 2].setColor(vec3(0.2f * 10.0f, 0.4f * 10.0f, (sin(seed + i) + 1.0f) * 10.0f / 2.0f));
		pointLightComponents[i + 3].setColor(vec3((sin(seed + i * 2.0 ) + 1.0f) * 10.0f / 2.0f, (sin(seed + i* 3.0) + 1.0f) * 10.0f / 2.0f, (sin(seed + i * 5.0) + 1.0f) * 10.0f / 2.0f));
	}
}

void InnocenceGarden::updateSpheres(float seed)
{
	for (auto i = (unsigned int)0; i < sphereActors.size(); i++)
	{
		//sphereActors[i].getTransform()->rotate(vec3(0.0f, 1.0f, 0.0f), 0.1 * i);
		//sphereActors[i].getTransform()->setPos(sphereActors[i].getTransform()->getPos() + vec3(cos(seed) * 0.1, 0.0, 0.0));
	}
}
