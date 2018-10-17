#pragma once
#include "BaseComponent.h"
#include "../common/InnoConcurrency.h"

class TaskSystemSingletonComponent : public BaseComponent
{
public:
	~TaskSystemSingletonComponent() {};
	
	static TaskSystemSingletonComponent& getInstance()
	{
		static TaskSystemSingletonComponent instance;
		return instance;
	}

	InnoThreadPool m_threadPool;
private:
	TaskSystemSingletonComponent() {};
};
