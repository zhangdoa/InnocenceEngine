#pragma once
#include "../common/InnoType.h"
#include <iostream>
#include "../common/InnoConcurrency.h"
class LogSystemSingletonComponent
{
public:
	~LogSystemSingletonComponent() {};
	
	static LogSystemSingletonComponent& getInstance()
	{
		static LogSystemSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

ThreadSafeQueue<std::string> m_log; 

private:
	LogSystemSingletonComponent() {};
};
