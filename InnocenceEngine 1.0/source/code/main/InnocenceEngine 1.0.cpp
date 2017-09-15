// InnocenceEngine 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../core/manager/CoreManager.h"
#include "../game/InnocenceGarden.h"


int main()
{
	
	InnocenceGarden* m_innocenceGarden = new InnocenceGarden();

	CoreManager::getInstance().setGameData(m_innocenceGarden);

	CoreManager::getInstance().excute(executeMessage::INITIALIZE);
	while (CoreManager::getInstance().getStatus() == objectStatus::ALIVE)
	{
		CoreManager::getInstance().excute(executeMessage::UPDATE);
	}
	CoreManager::getInstance().excute(executeMessage::SHUTDOWN);
	delete m_innocenceGarden;
	
	return EXIT_SUCCESS;
}

