#pragma once
#include "../Common/InnoType.h"
#include <iostream>
#include "../Common/InnoConcurrency.h"
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
