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
	//AssetManager::getInstance().importModel("nanosuit/nanosuit.blend");
	//AssetManager::getInstance().loadModel("nanosuit/nanosuit.innoModel", testStaticMeshComponent);

	rootActor.addChildActor(&playCharacter);
	//rootActor.addChildActor(&skyboxActor);
	rootActor.addChildActor(&testStaticMeshActor);

	//testSkyboxComponent.setVisiblilityType(visiblilityType::SKYBOX);
	//skyboxActor.addChildComponent(&testSkyboxComponent);
	//SceneGraphManager::getInstance().addToRenderingQueue(&testSkyboxComponent);
	//AssetManager::getInstance().loadTexture({ "skybox/right.jpg",
	//	"skybox/left.jpg", "skybox/top.jpg", "skybox/bottom.jpg", "skybox/back.jpg", "skybox/front.jpg" }, testSkyboxComponent);

	testStaticMeshComponent.setVisiblilityType(visiblilityType::STATIC_MESH);
	testStaticMeshActor.addChildComponent(&testStaticMeshComponent);
	SceneGraphManager::getInstance().addToRenderingQueue(&testStaticMeshComponent);
	AssetManager::getInstance().loadTexture("container.jpg", testStaticMeshComponent);
	
	playCharacter.getTransform()->setPos(glm::vec3(0.0f, 0.0f, 2.0f));
	rootActor.excute(executeMessage::INITIALIZE);
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
