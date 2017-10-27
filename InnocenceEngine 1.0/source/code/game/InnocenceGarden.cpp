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
	rootActor.addChildActor(&testLightActor1);
	rootActor.addChildActor(&testLightActor2);
	rootActor.addChildActor(&testLightActor3);

	rootActor.addChildActor(&testStaticMeshActor1);
	rootActor.addChildActor(&testStaticMeshActor2);

	testSkyboxComponent.setVisiblilityType(visiblilityType::SKYBOX);
	skyboxActor.addChildComponent(&testSkyboxComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&testSkyboxComponent);
	
	testLightActor1.addChildComponent(&testLightComponent1);
	SceneGraphManager::getInstance().addToLightQueue(&testLightComponent1);

	testLightBillboardComponent1.setVisiblilityType(visiblilityType::BILLBOARD);
	testLightActor1.addChildComponent(&testLightBillboardComponent1);
	SceneGraphManager::getInstance().addToRenderingQueue(&testLightBillboardComponent1);

	testLightActor2.addChildComponent(&testLightComponent2);
	SceneGraphManager::getInstance().addToLightQueue(&testLightComponent2);

	testLightBillboardComponent2.setVisiblilityType(visiblilityType::BILLBOARD);
	testLightActor2.addChildComponent(&testLightBillboardComponent2);
	SceneGraphManager::getInstance().addToRenderingQueue(&testLightBillboardComponent2);
	
	testLightActor3.addChildComponent(&testLightComponent3);
	SceneGraphManager::getInstance().addToLightQueue(&testLightComponent3);

	testLightBillboardComponent3.setVisiblilityType(visiblilityType::BILLBOARD);
	testLightActor3.addChildComponent(&testLightBillboardComponent3);
	SceneGraphManager::getInstance().addToRenderingQueue(&testLightBillboardComponent3);

	testStaticMeshComponent1.setVisiblilityType(visiblilityType::STATIC_MESH);
	testStaticMeshActor1.addChildComponent(&testStaticMeshComponent1);
	SceneGraphManager::getInstance().addToRenderingQueue(&testStaticMeshComponent1);

	testStaticMeshComponent2.setVisiblilityType(visiblilityType::STATIC_MESH);
	testStaticMeshActor2.addChildComponent(&testStaticMeshComponent2);
	SceneGraphManager::getInstance().addToRenderingQueue(&testStaticMeshComponent2);
	
	playCharacter.getTransform()->setPos(glm::vec3(0.0f, 2.0f, 5.0f));
	testLightActor1.getTransform()->setPos(glm::vec3(-2.0f, 1.0f, 2.0f));
	testLightActor2.getTransform()->setPos(glm::vec3(2.0f, 1.0f, 2.0f));
	testLightActor3.getTransform()->setPos(glm::vec3(0.0f, 1.0f, 4.0f));

	testStaticMeshActor1.getTransform()->setScale(glm::vec3(0.2f, 0.2f, 0.2f));
	testStaticMeshActor1.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 1.5f));
	testStaticMeshActor2.getTransform()->setScale(glm::vec3(0.002f, 0.002f, 0.002f));
	testStaticMeshActor2.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 3.5f));

	rootActor.excute(executeMessage::INITIALIZE);

	AssetManager::getInstance().importModel("nanosuit/nanosuit.obj");	
	AssetManager::getInstance().importModel("deer.obj");

	AssetManager::getInstance().loadModel("nanosuit/nanosuit.innoModel", testStaticMeshComponent1);
	AssetManager::getInstance().loadModel("deer.innoModel", testStaticMeshComponent2);

	AssetManager::getInstance().loadTexture({ "skybox2/right.tga",
		"skybox2/left.tga", "skybox2/top.tga", "skybox2/bottom.tga", "skybox2/back.tga", "skybox2/front.tga" }, testSkyboxComponent);

	AssetManager::getInstance().loadTexture("lightbulb.png", testLightBillboardComponent1);
	AssetManager::getInstance().loadTexture("lightbulb.png", testLightBillboardComponent2);
	AssetManager::getInstance().loadTexture("lightbulb.png", testLightBillboardComponent3);
}

void InnocenceGarden::update()
{
	temp += 0.05f;
	testLightComponent1.setColor(glm::vec3(0.0f, (glm::sin(temp) + 1.0f) / 2.0f, 0.0f));
	testLightComponent2.setColor(glm::vec3((glm::sin(temp) + 1.0f) / 2.0f, 0.0f, 0.0f));
	testLightComponent3.setColor(glm::vec3(0.0f, 0.0f, (glm::sin(temp) + 1.0f) / 2.0f));

	rootActor.excute(executeMessage::UPDATE);
}

void InnocenceGarden::shutdown()
{	
	rootActor.excute(executeMessage::SHUTDOWN);
}
