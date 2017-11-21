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

	rootActor.addChildActor(&landscapeStaticMeshActor);
	rootActor.addChildActor(&testStaticMeshActor1);
	rootActor.addChildActor(&testStaticMeshActor2);

	SceneGraphManager::getInstance().addToCameraQueue(&playCharacter.getCameraComponent());

	SceneGraphManager::getInstance().addToInputQueue(&playCharacter.getInputComponent());

	testSkyboxComponent.setVisiblilityType(visiblilityType::SKYBOX);
	testSkyboxComponent.setTextureWrapMethod(textureWrapMethod::CLAMPTOEDGE);
	skyboxActor.addChildComponent(&testSkyboxComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&testSkyboxComponent);
	
	directionalLightComponent.setlightType(lightType::DIRECTIONAL);
	directionalLightComponent.setDirection(glm::vec3(0.5f, 1.0f, 1.0f));
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

	landscapeStaticMeshComponent.addGraphicData();
	landscapeStaticMeshComponent.getGraphicData()[0].getMeshData().addTestCube();
	landscapeStaticMeshComponent.setVisiblilityType(visiblilityType::STATIC_MESH);
	landscapeStaticMeshActor.addChildComponent(&landscapeStaticMeshComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&landscapeStaticMeshComponent);

	testStaticMeshComponent1.setVisiblilityType(visiblilityType::STATIC_MESH);
	testStaticMeshActor1.addChildComponent(&testStaticMeshComponent1);
	SceneGraphManager::getInstance().addToRenderingQueue(&testStaticMeshComponent1);

	testStaticMeshComponent2.setVisiblilityType(visiblilityType::STATIC_MESH);
	testStaticMeshActor2.addChildComponent(&testStaticMeshComponent2);
	SceneGraphManager::getInstance().addToRenderingQueue(&testStaticMeshComponent2);
	
	playCharacter.getTransform()->setPos(glm::vec3(0.0f, 2.0f, 5.0f));
	pointLightActor1.getTransform()->setPos(glm::vec3(-2.0f, 1.0f, 2.0f));
	pointLightActor2.getTransform()->setPos(glm::vec3(2.0f, 1.0f, 2.0f));
	pointLightActor3.getTransform()->setPos(glm::vec3(0.0f, 1.0f, 4.0f));

	landscapeStaticMeshActor.getTransform()->setScale(glm::vec3(60.0f, 60.0f, 0.1f));
	landscapeStaticMeshActor.getTransform()->rotate(glm::vec3(1.0f, 0.0f, 0.0f), 90.0f);
	landscapeStaticMeshActor.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 0.0f));
	testStaticMeshActor1.getTransform()->setScale(glm::vec3(0.2f, 0.2f, 0.2f));
	testStaticMeshActor1.getTransform()->setPos(glm::vec3(0.0f, 0.0f, -1.5f));
	testStaticMeshActor2.getTransform()->setScale(glm::vec3(0.2f, 0.2f, 0.2f));
	//testStaticMeshActor2.getTransform()->setScale(glm::vec3(0.002f, 0.002f, 0.002f));
	testStaticMeshActor2.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 3.5f));

	rootActor.excute(executeMessage::INITIALIZE);

	AssetManager::getInstance().importModel("nanosuit/nanosuit.obj");	
	//AssetManager::getInstance().importModel("deer.obj");

	AssetManager::getInstance().loadModel("nanosuit/nanosuit.innoModel", testStaticMeshComponent1);
	AssetManager::getInstance().loadModel("nanosuit/nanosuit.innoModel", testStaticMeshComponent2);
	//AssetManager::getInstance().loadModel("deer.innoModel", testStaticMeshComponent2);

	AssetManager::getInstance().loadTexture({ "skybox2/right.tga",
		"skybox2/left.tga", "skybox2/top.tga", "skybox2/bottom.tga", "skybox2/back.tga", "skybox2/front.tga" }, testSkyboxComponent);

	AssetManager::getInstance().loadTexture("lightbulb.png", textureType::DIFFUSE, pointLightBillboardComponent1);
	AssetManager::getInstance().loadTexture("lightbulb.png", textureType::DIFFUSE, pointLightBillboardComponent2);
	AssetManager::getInstance().loadTexture("lightbulb.png", textureType::DIFFUSE, pointLightBillboardComponent3);

	AssetManager::getInstance().loadTexture("test_diffuse.png", textureType::DIFFUSE, landscapeStaticMeshComponent);
	AssetManager::getInstance().loadTexture("test_specular.png", textureType::SPECULAR, landscapeStaticMeshComponent);
	AssetManager::getInstance().loadTexture("test_normal.png", textureType::NORMALS, landscapeStaticMeshComponent);
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

	rootActor.excute(executeMessage::UPDATE);
}

void InnocenceGarden::shutdown()
{	
	rootActor.excute(executeMessage::SHUTDOWN);
}
