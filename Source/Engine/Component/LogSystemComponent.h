#pragma once
#include "../common/InnoType.h"
#include <iostream>
#include "../common/InnoConcurrency.h"
class LogSystemComponent
{
public:
	~LogSystemComponent() {};
	
	static LogSystemComponent& get()
	{
		static LogSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

ThreadSafeQueue<std::string> m_log; 

private:
	LogSystemComponent() {};
};
