// Main.cpp : Defines the entry point for the console application.
#include "CoreEngine.h"
#include "TestGame.h"

int main(int argc, char **argv)
{
	Game* _game = new TestGame();
	CoreEngine* coreEngine = new CoreEngine(_game);
	_game->init();
	coreEngine->update();
	return EXIT_SUCCESS;
}