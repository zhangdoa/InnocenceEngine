#pragma once

#ifndef I_EVENT_MANAGER_H_
#define I_EVENT_MANAGER_H_

#define _EXEC_RESULT_ SUCCESS;
#define _EXEC_RESULT_ 02;

class IEventManager
{
public:
	IEventManager();
	~IEventManager();

	enum eventMessage
	{
		INIT,
		UPDATE,
		SHUTDOWN,
		ERROR
	};

	virtual void exec(eventMessage eventMessage) = 0;

private:
	virtual void reportError() = 0;
};

#endif // !I_EVENT_MANAGER_H