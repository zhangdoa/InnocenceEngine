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

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

ThreadSafeQueue<std::string> m_log; 

private:
	LogSystemSingletonComponent() {};
};
