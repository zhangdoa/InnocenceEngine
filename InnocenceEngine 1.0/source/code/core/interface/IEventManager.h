#pragma once
#include "IBaseObject.h"

#ifndef _I_EVENT_MANAGER_H_
#define _I_EVENT_MANAGER_H_

class IEventManager : public IBaseObject
{
public:
	IEventManager();
	virtual ~IEventManager();
	virtual void setup() = 0;
};

#endif // !_I_EVENT_MANAGER_H_