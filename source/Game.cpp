#include "Game.h"



Game::Game()
{
	init();
}


Game::~Game()
{
	shutdown();
}

void Game::init()
{
	_rootGameObject = new GameObject();

	testMeshObject = new GameObject();
	testMeshObject->setName("testMeshObject");
	testMeshComponent = new StaticMeshComponent();
	testMeshComponent->setName("testMeshComponent");
	testMeshObject->addComponent(testMeshComponent);
	this->addObject(testMeshObject);
	testMeshObject->getTransform()->setPos(Vec3f(0.0f, -1.0f, 5.0f));

	_CameraObject = new GameObject();
	_CameraObject->setName("testCameraObject");
	_CameraComponent = new CameraComponent();
	_CameraComponent->setName("testCameraComponent");
	_CameraObject->addComponent(_CameraComponent);
	this->addObject(_CameraObject);
	fprintf(stdout, "Game has been initialized.\n");
}

void Game::input(float delta)
{
	_rootGameObject->input(delta);
}

void Game::update(float delta)
{
	//fprintf(stdout, "Game is updating.\n");
	_rootGameObject->update(delta);

}

void Game::render()
{
	//fprintf(stdout, "Game is rendering.\n");
	_renderingEngine->render(_rootGameObject);
}

void Game::shutdown()
{
	_rootGameObject = nullptr;
	delete _rootGameObject;
}

void Game::setRenderingEngine(RenderingEngine * renderingEngine)
{
	_renderingEngine = renderingEngine;
}

void Game::setrootGameObject(GameObject * rootGameObject)
{
	_rootGameObject = rootGameObject;
}

void Game::addObject(GameObject * object)
{
	_rootGameObject->addChind(object);
	_rootGameObject->setRenderingEngine(_renderingEngine);
}

CameraComponent * Game::getCameraComponent()
{
	return _CameraComponent;
}
