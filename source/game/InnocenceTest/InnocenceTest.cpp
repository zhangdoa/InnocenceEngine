#include "InnocenceTest.h"

void InnocenceTest::setup()
{
		GameSystem::setup();
		addComponentsToMap();
		testContainer();
		m_objectStatus = objectStatus::ALIVE;
}

void InnocenceTest::initialize()
{
	GameSystem::initialize();
}

void InnocenceTest::update()
{
	GameSystem::update();
}

void InnocenceTest::shutdown()
{
	GameSystem::shutdown();
}

const objectStatus & InnocenceTest::getStatus() const
{
	return m_objectStatus;
}

std::string InnocenceTest::getGameName() const
{
	return std::string{ typeid(*this).name() }.substr(std::string{ typeid(*this).name() }.find("class"), std::string::npos);
}

std::vector<TransformComponent*>& InnocenceTest::getTransformComponents()
{
	return m_transformComponents;
}

std::vector<CameraComponent*>& InnocenceTest::getCameraComponents()
{
	return m_cameraComponents;
}

std::vector<InputComponent*>& InnocenceTest::getInputComponents()
{
	return m_inputComponents;
}

std::vector<LightComponent*>& InnocenceTest::getLightComponents()
{
	return m_lightComponents;
}

std::vector<VisibleComponent*>& InnocenceTest::getVisibleComponents()
{
	return m_visibleComponents;
}

void InnocenceTest::testMath()
{
	auto t = g_pMemorySystem->spawn<TransformComponent>();
}

void InnocenceTest::testMemory()
{
}

void InnocenceTest::testConcurrency()
{
}

void InnocenceTest::testContainer()
{
	innoVector<int> l_innoVectorTest;
	for (int i = 0; i < 2048; i++)
	{
		l_innoVectorTest.push_back(i);
	}
	g_pLogSystem->printLog(l_innoVectorTest.size());
	for (int i = 0; i < l_innoVectorTest.size(); i++)
	{
		l_innoVectorTest.pop_back();
	}
	g_pLogSystem->printLog(l_innoVectorTest.size());
}
