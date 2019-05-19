#include "LogSystem.h"
#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoLogSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	std::string getLogTimeHeader()
	{
		auto l_timeData = g_pCoreSystem->getTimeSystem()->getCurrentTime();
		return
			"["
			+ std::to_string(l_timeData.year)
			+ "-" + std::to_string(l_timeData.month)
			+ "-" + std::to_string(l_timeData.day)
			+ "-" + std::to_string(l_timeData.hour)
			+ "-" + std::to_string(l_timeData.minute)
			+ "-" + std::to_string(l_timeData.second)
			+ "-" + std::to_string(l_timeData.millisecond)
			+ "]";
	}

#if defined INNO_PLATFORM_WIN
#include <windows.h>
	inline std::ostream& redColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_RED | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& greenColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& blueColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_BLUE | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& yellowColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
		return s;
	}
	inline std::ostream& whiteColor(std::ostream &s)
	{
		HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(hStdout,
			FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
		return s;
	}
#endif

	unsigned int m_logLevel = 0;
	std::ofstream m_logFile;

	std::mutex m_mutex;
}

void InnoLogSystem::printLog(double logMessage)
{
	std::cout << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl;
}

void InnoLogSystem::printLog(const vec2 & logMessage)
{
	std::cout
		<< InnoLogSystemNS::getLogTimeHeader()
		<< "vec2(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ")"
		<< std::endl;
}

void InnoLogSystem::printLog(const vec4 & logMessage)
{
	std::cout
		<< InnoLogSystemNS::getLogTimeHeader()
		<< "vec4(x: "
		<< logMessage.x
		<< ", y: "
		<< logMessage.y
		<< ", z: "
		<< logMessage.z
		<< ", w: "
		<< logMessage.w
		<< ")"
		<< std::endl;
}

void InnoLogSystem::printLog(const mat4 & logMessage)
{
	std::cout
		<< InnoLogSystemNS::getLogTimeHeader()
		<< std::endl
		<< "|"
		<< logMessage.m00
		<< ""
		<< logMessage.m10
		<< ""
		<< logMessage.m20
		<< ""
		<< logMessage.m30
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m01
		<< ""
		<< logMessage.m11
		<< ""
		<< logMessage.m21
		<< ""
		<< logMessage.m31
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m02
		<< ""
		<< logMessage.m12
		<< ""
		<< logMessage.m22
		<< ""
		<< logMessage.m32
		<< "|"
		<< std::endl
		<< "|"
		<< logMessage.m03
		<< ""
		<< logMessage.m13
		<< ""
		<< logMessage.m23
		<< ""
		<< logMessage.m33
		<< "|"
		<< std::endl;
}

void InnoLogSystem::printLog(LogType LogType, const std::string & logMessage)
{
	std::lock_guard<std::mutex> lock{ InnoLogSystemNS::m_mutex };
#if defined INNO_PLATFORM_WIN
	switch (LogType)
	{
	case LogType::INNO_DEV_VERBOSE:
		if (InnoLogSystemNS::m_logLevel == 1)
		{
			std::cout << InnoLogSystemNS::blueColor << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl;
		}
		break;
	case LogType::INNO_WARNING:
		std::cout << InnoLogSystemNS::yellowColor << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl; break;
	case LogType::INNO_ERROR:
		std::cout << InnoLogSystemNS::redColor << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl; break;
	case LogType::INNO_DEV_SUCCESS:
		std::cout << InnoLogSystemNS::greenColor << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl; break;
	default: std::cout << InnoLogSystemNS::whiteColor << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl; break;
	}
#else
	std::cout << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl;
#endif
	InnoLogSystemNS::m_logFile << InnoLogSystemNS::getLogTimeHeader() << logMessage << std::endl;
}

ObjectStatus InnoLogSystem::getStatus()
{
	return InnoLogSystemNS::m_objectStatus;
}

bool InnoLogSystem::setup()
{
	return true;
}

bool InnoLogSystem::initialize()
{
	InnoLogSystemNS::m_logFile.open(g_pCoreSystem->getFileSystem()->getWorkingDirectory() + "res//logs//" + InnoLogSystemNS::getLogTimeHeader() + ".log", std::ios::out | std::ios::trunc);

	InnoLogSystemNS::m_objectStatus = ObjectStatus::ALIVE;
	printLog(LogType::INNO_DEV_SUCCESS, "LogSystem has been initialized.");
	return true;
}

bool InnoLogSystem::update()
{
	return true;
}

bool InnoLogSystem::terminate()
{
	InnoLogSystemNS::m_logFile.close();
	InnoLogSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	printLog(LogType::INNO_DEV_SUCCESS, "LogSystem has been terminated.");
	return true;
}