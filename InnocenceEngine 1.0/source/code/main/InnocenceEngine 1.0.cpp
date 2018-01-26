// InnocenceEngine 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../core/data/InnoMath.h"
#include "../core/manager/CoreManager.h"
#include "../game/InnocenceGarden.h"
#include "../game/MathTest.h"

void startEngine()
{
	InnocenceGarden* m_innocenceGarden = new InnocenceGarden();
	CoreManager::getInstance().setGameData(m_innocenceGarden);
	//MathTest* m_MathTest = new MathTest();
	//CoreManager::getInstance().setGameData(m_MathTest);

	CoreManager::getInstance().setup();
	CoreManager::getInstance().initialize();

	while (CoreManager::getInstance().getStatus() == objectStatus::ALIVE)
	{
		CoreManager::getInstance().update();
	}
	CoreManager::getInstance().shutdown();
	delete m_innocenceGarden;
	//delete m_MathTest;
}

int main()
{
	startEngine();
	return EXIT_SUCCESS;
}

