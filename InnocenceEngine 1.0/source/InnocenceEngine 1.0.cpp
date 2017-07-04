// InnocenceEngine 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "IEventManager.h"
#include "CoreManager.h"


int main()
{
	CoreManager* _coreManager = new CoreManager();
	_coreManager->exec(IEventManager::INIT);
	while (_coreManager->getStatus() == IEventManager::INITIALIZIED)
	{
		_coreManager->exec(IEventManager::UPDATE);
	}
	_coreManager->exec(IEventManager::SHUTDOWN);
	return EXIT_SUCCESS;
}

