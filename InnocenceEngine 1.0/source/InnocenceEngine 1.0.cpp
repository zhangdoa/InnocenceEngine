// InnocenceEngine 1.0.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "IBaseObject.h"
#include "CoreManager.h"
#include "InnocenceGarden.h"


int main()
{
	CoreManager* m_coreManager = new CoreManager();
	InnocenceGarden* m_innocenceGarden = new InnocenceGarden();

	m_coreManager->setGameData(m_innocenceGarden);

	m_coreManager->exec(IBaseObject::INIT);
	while (m_coreManager->getStatus() == IBaseObject::INITIALIZIED)
	{
		m_coreManager->exec(IBaseObject::UPDATE);
	}
	m_coreManager->exec(IBaseObject::SHUTDOWN);
	return EXIT_SUCCESS;
}

