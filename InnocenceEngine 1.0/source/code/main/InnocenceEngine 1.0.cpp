// InnocenceEngine 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../core/manager/CoreManager.h"
#include "../game/InnocenceGarden.h"


int main()
{
	
	InnocenceGarden* m_innocenceGarden = new InnocenceGarden();

	CoreManager::getInstance().setGameData(m_innocenceGarden);

	CoreManager::getInstance().exec(execMessage::INIT);
	while (CoreManager::getInstance().getStatus() == objectStatus::INITIALIZIED)
	{
		CoreManager::getInstance().exec(execMessage::UPDATE);
	}
	CoreManager::getInstance().exec(execMessage::SHUTDOWN);
	delete m_innocenceGarden;
	
	return EXIT_SUCCESS;
}

