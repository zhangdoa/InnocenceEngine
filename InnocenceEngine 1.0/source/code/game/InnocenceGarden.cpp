#include "../main/stdafx.h"
#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}

InnocenceGarden::~InnocenceGarden()
{
}

void InnocenceGarden::init()
{	
	//AssetManager::getInstance().loadModel("nanosuit/nanosuit.blend");
	rootActor.addChildActor(&playCharacter);
	rootActor.addChildActor(&skyboxActor);
	rootActor.addChildActor(&testStaticMeshActor);

	skyboxActor.addChildComponent(&testSkyboxComponent);
	testStaticMeshActor.addChildComponent(&testStaticMeshComponent);
	rootActor.exec(execMessage::INIT);
	testStaticMeshComponent.loadMesh("nanosuit/nanosuit_c11_m0.innoMesh");
	testStaticMeshComponent.loadTexture("nanosuit/body_dif.png");

	testStaticMeshActor.getTransform()->setPos(glm::vec3(0.0f, 0.0f, -5.0f));
}

void InnocenceGarden::update()
{
	//testTriangleActor.getTransform()->rotate(glm::vec3(-1.0f, 0.0f, 0.0f), 1.0);
	//testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, -1.0f, 0.0f), 0.5);
	//testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, 0.0f, -1.0f), 0.25);

	rootActor.exec(execMessage::UPDATE);
}

void InnocenceGarden::shutdown()
{	
	rootActor.exec(execMessage::SHUTDOWN);
}
