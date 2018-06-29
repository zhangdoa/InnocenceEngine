#include "InnocenceTest.h"


InnocenceTest::InnocenceTest()
{
}


InnocenceTest::~InnocenceTest()
{
}

void InnocenceTest::setup()
{
	m_rootEntity.setup();
}

void InnocenceTest::initialize()
{
	m_rootEntity.initialize();
	testMath();
	g_pMemorySystem->dumpToFile(false);
}

void InnocenceTest::update()
{
}

void InnocenceTest::shutdown()
{
	m_rootEntity.shutdown();
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
	//innoVector<int> l_innoVectorTest;
	//for (int i = 0; i < 10; i++)
	//{
	//	l_innoVectorTest.push_back(i);
	//	g_pLogSystem->printLog(l_innoVectorTest.size());
	//}
	auto t = g_pMemorySystem->spawn<TransformComponent>();
}

void InnocenceTest::testMemory()
{

}
