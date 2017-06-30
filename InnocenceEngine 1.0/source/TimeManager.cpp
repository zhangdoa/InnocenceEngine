#include "stdafx.h"
#include "TimeManager.h"


TimeManager::TimeManager()
{
}


TimeManager::~TimeManager()
{
}

void TimeManager::exec(eventMessage eventMessage)
{
	switch (eventMessage)
	{
	case INIT: init(); break;
	case UPDATE : update(); break;
	case SHUTDOWN: shutdown(); break;
	default: reportError();
		break;
	}
}

void TimeManager::init()
{
}

void TimeManager::update()
{
}

void TimeManager::shutdown()
{
}

void TimeManager::reportError()
{
}
