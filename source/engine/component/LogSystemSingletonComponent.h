#pragma once
#include "BaseComponent.h"

#include "../common/InnoConcurrency.h"
class LogSystemSingletonComponent : public BaseComponent
{
public:
	~LogSystemSingletonComponent() {};
	
	static LogSystemSingletonComponent& getInstance()
	{
		static LogSystemSingletonComponent instance;
		return instance;
	}

ThreadSafeQueue<std::string> m_log; 

private:
	LogSystemSingletonComponent() {};
};
