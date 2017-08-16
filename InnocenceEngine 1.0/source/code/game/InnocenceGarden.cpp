#include "../main/stdafx.h"
#include "InnocenceGarden.h"


InnocenceGarden::InnocenceGarden()
{
}


InnocenceGarden::~InnocenceGarden()
{
}

CameraComponent * InnocenceGarden::getCameraComponent()
{
	return &testCameraComponent;
}

IVisibleGameEntity * InnocenceGarden::getSkybox()
{
	return &testSkyboxComponent;
}

IVisibleGameEntity * InnocenceGarden::getTestStaticMeshComponent()
{
	return &testStaticMeshComponent;
}

void InnocenceGarden::init()
{	
	//AssetManager::getInstance().loadModel("nanosuit/nanosuit.blend");
	testRootActor.addChildActor(&testCameraActor);
	testRootActor.addChildActor(&testSkyboxActor);
	testRootActor.addChildActor(&testStaticMeshActor);

	testCameraActor.addChildComponent(&testCameraComponent);
	testSkyboxActor.addChildComponent(&testSkyboxComponent);
	testStaticMeshActor.addChildComponent(&testStaticMeshComponent);
	testRootActor.exec(execMessage::INIT);
	testStaticMeshComponent.loadMesh("nanosuit/nanosuit_c11_m0.innoMesh");
	testStaticMeshComponent.loadTexture("nanosuit/body_dif.png");

	testStaticMeshActor.getTransform()->setPos(glm::vec3(0.0f, 0.0f, -5.0f));
}

void InnocenceGarden::update()
{
	//testTriangleActor.getTransform()->rotate(glm::vec3(-1.0f, 0.0f, 0.0f), 1.0);
	//testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, -1.0f, 0.0f), 0.5);
	//testTriangleActor.getTransform()->rotate(glm::vec3(0.0f, 0.0f, -1.0f), 0.25);

	testRootActor.exec(execMessage::UPDATE);
}

void InnocenceGarden::shutdown()
{	
	testRootActor.exec(execMessage::SHUTDOWN);
}
