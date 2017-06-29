// Main.cpp : Defines the entry point for the console application.
#include "CoreEngine.h"

int main(int argc, char **argv)
{
	CoreEngine* coreEngine = new CoreEngine();
	coreEngine->update();
	return EXIT_SUCCESS;
}