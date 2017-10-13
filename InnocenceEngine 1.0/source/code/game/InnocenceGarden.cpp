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
	rootActor.addChildActor(&testStaticMeshActor1);
	rootActor.addChildActor(&testStaticMeshActor2);

	testSkyboxComponent.setVisiblilityType(visiblilityType::SKYBOX);
	skyboxActor.addChildComponent(&testSkyboxComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&testSkyboxComponent);
	
	testStaticMeshComponent1.setVisiblilityType(visiblilityType::STATIC_MESH);
	testStaticMeshActor1.addChildComponent(&testStaticMeshComponent1);
	SceneGraphManager::getInstance().addToRenderingQueue(&testStaticMeshComponent1);

	testStaticMeshComponent2.setVisiblilityType(visiblilityType::STATIC_MESH);
	testStaticMeshActor2.addChildComponent(&testStaticMeshComponent2);
	SceneGraphManager::getInstance().addToRenderingQueue(&testStaticMeshComponent2);
	
	playCharacter.getTransform()->setPos(glm::vec3(0.0f, 2.0f, 5.0f));
	testStaticMeshActor1.getTransform()->setScale(glm::vec3(0.2f, 0.2f, 0.2f));
	testStaticMeshActor1.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 1.5f));
	testStaticMeshActor2.getTransform()->setScale(glm::vec3(0.05f, 0.05f, 0.05f));

	rootActor.excute(executeMessage::INITIALIZE);

	AssetManager::getInstance().importModel("nanosuit/nanosuit.obj");
	AssetManager::getInstance().loadModel("nanosuit/nanosuit.innoModel", testStaticMeshComponent1);

	//AssetManager::getInstance().importModel("bugatti/bugatti.obj");
	//AssetManager::getInstance().loadModel("bugatti/bugatti.innoModel", testStaticMeshComponent2);

	AssetManager::getInstance().loadTexture({ "skybox2/right.tga",
		"skybox2/left.tga", "skybox2/top.tga", "skybox2/bottom.tga", "skybox2/back.tga", "skybox2/front.tga" }, testSkyboxComponent);
}

void InnocenceGarden::update()
{
	//testTriangleActor.getTransform()->rotate(glm::vec3(-1.0f, 0.0f, 0.0f), 1.0);
	//testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, -1.0f, 0.0f), 0.5);
	//testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, 0.0f, -1.0f), 0.25);
	rootActor.excute(executeMessage::UPDATE);
}

void InnocenceGarden::shutdown()
{	
	rootActor.excute(executeMessage::SHUTDOWN);
}
