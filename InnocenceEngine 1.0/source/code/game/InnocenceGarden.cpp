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
	AssetManager::getInstance().importModel("nanosuit/nanosuit.blend");

	rootActor.addChildActor(&playCharacter);
	rootActor.addChildActor(&skyboxActor);
	rootActor.addChildActor(&testStaticMeshActor);

	skyboxActor.addChildComponent(&testSkyboxComponent);
	testStaticMeshActor.addChildComponent(&testStaticMeshComponent);

	rootActor.excute(executeMessage::INITIALIZE);
	testStaticMeshComponent.loadModel("nanosuit/nanosuit.innoModel");
	//testStaticMeshComponent.loadTexture("nanosuit/body_dif.png");

	testStaticMeshActor.getTransform()->setPos(glm::vec3(0.0f, 0.0f, -5.0f));
	testStaticMeshActor.getTransform()->setScale(glm::vec3(0.1f, 0.1f, 0.1f));
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
