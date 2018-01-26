#include "../main/stdafx.h"
#include "MathTest.h"


MathTest::MathTest()
{
}


MathTest::~MathTest()
{
}

void MathTest::setup()
{
}

void MathTest::initialize()
{
	LogManager::getInstance().printLog(vec3(1.0, 0.0, 0.0).cross(vec3(0.0, 1.0, 0.0)));
}

void MathTest::update()
{
}

void MathTest::shutdown()
{
}