#include "stdafx.h"
#include "LogManager.h"
#include "TimeManager.h"


LogManager::LogManager()
{
}


LogManager::~LogManager()
{
}

void LogManager::printLog(std::string logMessage)
{
	std::cout << "[" << TimeManager::getCurrentTimeInLocal()  << "]" << logMessage << std::endl;
}

void LogManager::printLog(const Vec2f & logMessage)
{
	std::cout
		<< "[" 
		<< TimeManager::getCurrentTimeInLocal() 
		<< "]"
		<<"Vec2f(x: "
		<< logMessage.getX()
		<<", y: "
		<< logMessage.getY()
		<<")"
		<< std::endl;
}

void LogManager::printLog(const Vec3f & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getCurrentTimeInLocal()
		<< "]"
		<< "Vec2f(x: "
		<< logMessage.getX()
		<< ", y: "
		<< logMessage.getY()
		<< ", z: "
		<< logMessage.getZ()
		<< ")"
		<< std::endl;
}

void LogManager::printLog(const Vec4f & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getCurrentTimeInLocal()
		<< "]"
		<< "Vec2f(x: "
		<< logMessage.getX()
		<< ", y: "
		<< logMessage.getY()
		<< ", z: "
		<< logMessage.getZ()
		<< ", w: "
		<< logMessage.getW()
		<< ")"
		<< std::endl;
}

void LogManager::printLog(const Mat4f & logMessage)
{
	std::cout
		<< "["
		<< TimeManager::getCurrentTimeInLocal()
		<< "]"
		<< std::endl
		<< "|"
		<< logMessage.getElem(0, 0)
		<< ", "
		<< logMessage.getElem(0, 1)
		<< ", "
		<< logMessage.getElem(0, 2)
		<< ", "
		<< logMessage.getElem(0, 3)
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.getElem(1, 0)
		<< ", "
		<< logMessage.getElem(1, 1)
		<< ", "
		<< logMessage.getElem(1, 2)
		<< ", "
		<< logMessage.getElem(1, 3)
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.getElem(2, 0)
		<< ", "
		<< logMessage.getElem(2, 1)
		<< ", "
		<< logMessage.getElem(2, 2)
		<< ", "
		<< logMessage.getElem(2, 3)
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.getElem(3, 0)
		<< ", "
		<< logMessage.getElem(3, 1)
		<< ", "
		<< logMessage.getElem(3, 2)
		<< ", "
		<< logMessage.getElem(3, 3)
		<< "|"
		<< std::endl;
}

void LogManager::init()
{
}

void LogManager::update()
{
}

void LogManager::shutdown()
{
}
