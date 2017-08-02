#pragma once

#include "../interface/IBaseObject.h"

#ifndef _I_EVENT_MANAGER_H_
#define _I_EVENT_MANAGER_H_

class IEventManager : public IBaseObject
{
public:
	IEventManager();
	virtual ~IEventManager();
};

#endif // !_I_EVENT_MANAGER_H_