#pragma once
#include "common/stdafx.h"
#include "ISystem.h"

class ITaskSystem : public ISystem
{
public:
	virtual ~ITaskSystem() {};

	virtual void addTask(void* task) = 0;
};