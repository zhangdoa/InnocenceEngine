#pragma once
#include "../common/InnoType.h"
#include "../common/InnoConcurrency.h"

class TaskSystemSingletonComponent
{
public:
	~TaskSystemSingletonComponent() {};
	
	static TaskSystemSingletonComponent& getInstance()
	{
		static TaskSystemSingletonComponent instance;
		return instance;
	}

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	InnoThreadPool m_threadPool;
private:
	TaskSystemSingletonComponent() {};
};
