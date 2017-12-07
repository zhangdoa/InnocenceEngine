#include "../main/stdafx.h"
#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}

InnocenceGarden::~InnocenceGarden()
{
}

void InnocenceGarden::initialize()
{	
	rootActor.addChildActor(&playCharacter);

	rootActor.addChildActor(&skyboxActor);

	rootActor.addChildActor(&directionalLightActor);
	rootActor.addChildActor(&pointLightActor1);
	rootActor.addChildActor(&pointLightActor2);
	rootActor.addChildActor(&pointLightActor3);

	rootActor.addChildActor(&landscapeActor);
	rootActor.addChildActor(&pawnActor1);
	rootActor.addChildActor(&pawnActor2);

	SceneGraphManager::getInstance().addToCameraQueue(&playCharacter.getCameraComponent());

	SceneGraphManager::getInstance().addToInputQueue(&playCharacter.getInputComponent());

	skyboxComponent.setVisiblilityType(visiblilityType::SKYBOX);
	skyboxComponent.setTextureWrapMethod(textureWrapMethod::CLAMPTOEDGE);
	skyboxActor.addChildComponent(&skyboxComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&skyboxComponent);
	
	directionalLightComponent.setlightType(lightType::DIRECTIONAL);
	directionalLightComponent.setDirection(glm::vec3(0.25f, 1.0f, 1.0f));
	directionalLightComponent.setAmbientColor(glm::vec3(0.0f, 0.35f, 0.55f));
	directionalLightComponent.setDiffuseColor(glm::vec3(0.0f, 0.35f, 0.55f));
	directionalLightComponent.setSpecularColor(glm::vec3(0.0f, 0.35f, 0.55f));
	directionalLightActor.addChildComponent(&directionalLightComponent);
	SceneGraphManager::getInstance().addToLightQueue(&directionalLightComponent);

	pointLightActor1.addChildComponent(&pointLightComponent1);
	SceneGraphManager::getInstance().addToLightQueue(&pointLightComponent1);

	pointLightBillboardComponent1.setVisiblilityType(visiblilityType::BILLBOARD);
	pointLightBillboardComponent1.setTextureWrapMethod(textureWrapMethod::CLAMPTOEDGE);
	pointLightActor1.addChildComponent(&pointLightBillboardComponent1);
	SceneGraphManager::getInstance().addToRenderingQueue(&pointLightBillboardComponent1);

	pointLightActor2.addChildComponent(&pointLightComponent2);
	SceneGraphManager::getInstance().addToLightQueue(&pointLightComponent2);

	pointLightBillboardComponent2.setVisiblilityType(visiblilityType::BILLBOARD);
	pointLightBillboardComponent2.setTextureWrapMethod(textureWrapMethod::CLAMPTOEDGE);
	pointLightActor2.addChildComponent(&pointLightBillboardComponent2);
	SceneGraphManager::getInstance().addToRenderingQueue(&pointLightBillboardComponent2);
	
	pointLightActor3.addChildComponent(&pointLightComponent3);
	SceneGraphManager::getInstance().addToLightQueue(&pointLightComponent3);

	pointLightBillboardComponent3.setVisiblilityType(visiblilityType::BILLBOARD);
	pointLightBillboardComponent3.setTextureWrapMethod(textureWrapMethod::CLAMPTOEDGE);
	pointLightActor3.addChildComponent(&pointLightBillboardComponent3);
	SceneGraphManager::getInstance().addToRenderingQueue(&pointLightBillboardComponent3);

	landscapeStaticMeshComponent.setVisiblilityType(visiblilityType::STATIC_MESH);
	landscapeActor.addChildComponent(&landscapeStaticMeshComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&landscapeStaticMeshComponent);

	pawnMeshComponent1.setVisiblilityType(visiblilityType::STATIC_MESH);
	pawnActor1.addChildComponent(&pawnMeshComponent1);
	SceneGraphManager::getInstance().addToRenderingQueue(&pawnMeshComponent1);

	pawnMeshComponent2.setVisiblilityType(visiblilityType::STATIC_MESH);
	pawnActor2.addChildComponent(&pawnMeshComponent2);
	SceneGraphManager::getInstance().addToRenderingQueue(&pawnMeshComponent2);
	
	playCharacter.getTransform()->setPos(glm::vec3(0.0f, 2.0f, 5.0f));
	directionalLightActor.getTransform()->setPos(glm::vec3(0.0f, 4.0f, -10.0f));
	pointLightActor1.getTransform()->setPos(glm::vec3(-2.0f, 1.0f, 2.0f));
	pointLightActor2.getTransform()->setPos(glm::vec3(2.0f, 1.0f, 2.0f));
	pointLightActor3.getTransform()->setPos(glm::vec3(0.0f, 1.0f, 4.0f));

	landscapeActor.getTransform()->setScale(glm::vec3(60.0f, 60.0f, 0.1f));
	landscapeActor.getTransform()->rotate(glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);
	landscapeActor.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 0.0f));
	pawnActor1.getTransform()->setScale(glm::vec3(0.2f, 0.2f, 0.2f));
	pawnActor1.getTransform()->setPos(glm::vec3(0.0f, 0.0f, -1.5f));
	pawnActor2.getTransform()->setScale(glm::vec3(0.2f, 0.2f, 0.2f));
	//testStaticMeshActor2.getTransform()->setScale(glm::vec3(0.002f, 0.002f, 0.002f));
	pawnActor2.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 3.5f));

	initSpheres();

	AssetManager::getInstance().importModel("nanosuit/nanosuit.obj");	
	//AssetManager::getInstance().importModel("deer.obj");

	AssetManager::getInstance().loadModel("nanosuit/nanosuit.innoModel", pawnMeshComponent1);
	AssetManager::getInstance().loadModel("nanosuit/nanosuit.innoModel", pawnMeshComponent2);
	//AssetManager::getInstance().loadModel("deer.innoModel", testStaticMeshComponent2);
	//AssetManager::getInstance().addUnitCube(skyboxComponent);
	//AssetManager::getInstance().addUnitQuad(pointLightBillboardComponent1);
	//AssetManager::getInstance().addUnitQuad(pointLightBillboardComponent2);
	//AssetManager::getInstance().addUnitQuad(pointLightBillboardComponent3);
	AssetManager::getInstance().addUnitCube(landscapeStaticMeshComponent);

	//AssetManager::getInstance().loadTexture({ "skybox2/right.tga",
	//	"skybox2/left.tga", "skybox2/top.tga", "skybox2/bottom.tga", "skybox2/back.tga", "skybox2/front.tga" }, skyboxComponent);

	//AssetManager::getInstance().loadTexture("lightbulb.png", textureType::DIFFUSE, pointLightBillboardComponent1);
	//AssetManager::getInstance().loadTexture("lightbulb.png", textureType::DIFFUSE, pointLightBillboardComponent2);
	//AssetManager::getInstance().loadTexture("lightbulb.png", textureType::DIFFUSE, pointLightBillboardComponent3);

	AssetManager::getInstance().loadTexture("test_diffuse.png", textureType::DIFFUSE, landscapeStaticMeshComponent);
	AssetManager::getInstance().loadTexture("test_specular.png", textureType::SPECULAR, landscapeStaticMeshComponent);
	AssetManager::getInstance().loadTexture("test_normal.png", textureType::NORMALS, landscapeStaticMeshComponent);

	loadTextureForSpheres();

	rootActor.initialize();
}

void InnocenceGarden::update()
{
	temp += 0.02f;
	pointLightComponent1.setAmbientColor(glm::vec3(0.0f, (glm::sin(temp) + 1.0f) / 2.0f, 0.0f));
	pointLightComponent1.setDiffuseColor(glm::vec3(0.0f, (glm::sin(temp) + 1.0f) / 2.0f, 0.0f));
	pointLightComponent1.setSpecularColor(glm::vec3(0.0f, (glm::sin(temp) + 1.0f) / 2.0f, 0.0f));
	pointLightActor1.getTransform()->setPos(glm::vec3(-2.0f, 1.0f, 2.0f) + glm::vec3(glm::sin(temp) + 1.0f, 0.0f, -glm::cos(temp) * 3.0f));

	pointLightComponent2.setAmbientColor(glm::vec3((glm::sin(temp * 2) + 1.0f) / 2.0f, 0.0f, 0.0f));
	pointLightComponent2.setDiffuseColor(glm::vec3((glm::sin(temp * 2) + 1.0f) / 2.0f, 0.0f, 0.0f));
	pointLightComponent2.setSpecularColor(glm::vec3((glm::sin(temp * 2) + 1.0f) / 2.0f, 0.0f, 0.0f));
	pointLightActor2.getTransform()->setPos(glm::vec3(2.0f, 1.0f, 2.0f) + glm::vec3(glm::sin(temp) + 1.0f, 0.0f, -glm::cos(temp) * 2.0f));

	pointLightComponent3.setAmbientColor(glm::vec3(0.0f, 0.0f, (glm::sin(temp * 3) + 1.0f) / 2.0f));
	pointLightComponent3.setDiffuseColor(glm::vec3(0.0f, 0.0f, (glm::sin(temp * 3) + 1.0f) / 2.0f));
	pointLightComponent3.setSpecularColor(glm::vec3(0.0f, 0.0f, (glm::sin(temp * 3) + 1.0f) / 2.0f));
	pointLightActor3.getTransform()->setPos(glm::vec3(0.0f, 1.0f, 4.0f) + glm::vec3(glm::sin(temp) + 1.0f, 0.0f, -glm::cos(temp) * 1.0f));

	rootActor.update();
}

void InnocenceGarden::shutdown()
{	
	rootActor.shutdown();
}

void InnocenceGarden::initSpheres()
{
	for (auto i = (unsigned int)0; i < sphereComponents.size(); i++)
	{
		BaseActor newBaseActor;
		sphereActors.emplace_back(newBaseActor);
		VisibleComponent newVisibleComponent;
		sphereComponents.emplace_back(newVisibleComponent);
		sphereComponents[i].setMeshDrawMethod(meshDrawMethod::TRIANGLE_STRIP);
		sphereComponents[i].setVisiblilityType(visiblilityType::STATIC_MESH);

		rootActor.addChildActor(&sphereActors[i]);
		sphereActors[i].addChildComponent(&sphereComponents[i]);

		SceneGraphManager::getInstance().addToRenderingQueue(&sphereComponents[i]);

		sphereActors[i].getTransform()->setPos(glm::vec3(-16 + i * 4, 2.0, -6.0));

		AssetManager::getInstance().addUnitCube(sphereComponents[i]);
	}
}

void InnocenceGarden::loadTextureForSpheres()
{
	for (int i = 0; i < 8; i++)
	{
		//AssetManager::getInstance().loadTexture("pbr_basecolor.png", textureType::DIFFUSE, sphereComponents[i]);
		//AssetManager::getInstance().loadTexture("pbr_metallic.png", textureType::SPECULAR, sphereComponents[i]);
		//AssetManager::getInstance().loadTexture("pbr_normal.png", textureType::NORMALS, sphereComponents[i]);
	}
	
}
