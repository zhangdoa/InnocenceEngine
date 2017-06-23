#include "Game.h"



Game::Game()
{
}


Game::~Game()
{
	shutdown();
}

void Game::init()
{
	_root = new GameObject();
	fprintf(stdout, "Game has been initialized.\n");
}

void Game::input(float delta)
{
	_root->input(delta);
}

void Game::update(float delta)
{
	_root->update(delta);
	//fprintf(stdout, "Game is updating.\n");
}

void Game::render(RenderingEngine * renderingEngine)
{
	fprintf(stdout, "Game is rendering.\n");
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
