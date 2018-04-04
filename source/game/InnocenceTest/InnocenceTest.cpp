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
}

void InnocenceTest::update()
{
	m_rootEntity.update();
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
	//LogSystem::getInstance().printLog(vec4(1.0, 0.0, 0.0).cross(vec4(0.0, 1.0, 0.0)));
}

void InnocenceTest::testMemory()
{

}
