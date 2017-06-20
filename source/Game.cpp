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
	_root = new GameObject();
}

void Game::input(float delta)
{
	_root->input(delta);
}

void Game::render(RenderingEngine * renderingEngine)
{
	renderingEngine->render(_root);
}

void Game::shutdown()
{
	_root = nullptr;
	delete _root;
}

void Game::addObject(GameObject * object)
{
	_root->addChind(object);
}
