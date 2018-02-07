// InnocenceEngine 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../core/data/InnoMath.h"
#include "../core/manager/CoreManager.h"
#include "../game/InnocenceGarden.h"
#include "../game/TestCase.h"

void startEngine()
{
	InnocenceGarden* m_innocenceGarden = new InnocenceGarden();
	CoreManager::getInstance().setGameData(m_innocenceGarden);
	//TestCase* m_testCase = new TestCase();
	//m_testCase->needRender = false;
	//CoreManager::getInstance().setGameData(m_testCase);

	CoreManager::getInstance().setup();
	CoreManager::getInstance().initialize();

	while (CoreManager::getInstance().getStatus() == objectStatus::ALIVE)
	{
		CoreManager::getInstance().update();
	}
	CoreManager::getInstance().shutdown();
	delete m_innocenceGarden;
	//delete m_testCase;
}

int main()
{
	startEngine();
	return EXIT_SUCCESS;
}

