#include "TestCase.h"


TestCase::TestCase()
{
}


TestCase::~TestCase()
{
}

void TestCase::setup()
{
}

void TestCase::initialize()
{
	testMath();
}

void TestCase::update()
{
}

void TestCase::shutdown()
{
}

void TestCase::testMath()
{
	//LogSystem::getInstance().printLog(vec3(1.0, 0.0, 0.0).cross(vec3(0.0, 1.0, 0.0)));
}

void TestCase::testMemory()
{

}
