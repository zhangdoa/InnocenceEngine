#include "InnocenceTest.h"

#include "../../engine/system/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

IGameInstance* g_pGameInstance;

namespace InnocenceTestNS
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void testMath();
	void testMemory();
	void testConcurrency();
	void testContainer();
}

InnocenceTest::InnocenceTest(void)
{
}

INNO_SYSTEM_EXPORT bool InnocenceTest::setup()
{
	InnocenceTestNS::m_objectStatus = objectStatus::ALIVE;
}

INNO_SYSTEM_EXPORT bool InnocenceTest::initialize()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnocenceTest::update()
{
	return true;
}

INNO_SYSTEM_EXPORT bool InnocenceTest::terminate()
{
	InnocenceTestNS::m_objectStatus = objectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT objectStatus InnocenceTest::getStatus()
{
	return InnocenceTestNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT std::string InnocenceTest::getGameName()
{
	return std::string("InnocenceTest");
}

void InnocenceTestNS::testMath()
{
}

void InnocenceTestNS::testMemory()
{
}

void InnocenceTestNS::testConcurrency()
{
}

void InnocenceTestNS::testContainer()
{
	innoVector<int> l_innoVectorTest;
	for (int i = 0; i < 2048; i++)
	{
		l_innoVectorTest.push_back(i);
	}
	g_pCoreSystem->getLogSystem()->printLog(l_innoVectorTest.size());
	for (int i = 0; i < l_innoVectorTest.size(); i++)
	{
		l_innoVectorTest.pop_back();
	}
	g_pCoreSystem->getLogSystem()->printLog(l_innoVectorTest.size());
}