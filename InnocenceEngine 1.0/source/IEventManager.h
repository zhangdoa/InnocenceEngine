#pragma once

#ifndef I_EVENT_MANAGER_H_
#define I_EVENT_MANAGER_H_

class IEventManager
{
public:
	IEventManager();
	virtual ~IEventManager();

	enum execMessage
	{
		INIT,
		UPDATE,
		SHUTDOWN,
		DEFAULT
	};

	enum managerStatus
	{
		STANDBY,
		RUNNING,
		UNINITIALIZIED,
		ERROR
	};


	void exec(execMessage execMessage);
	int getStatus();

protected:
	void setStatus(managerStatus managerStatus);
	void printLog(std::string logMessage);

private:
	managerStatus m_managerStatus = UNINITIALIZIED;

	virtual void init() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;

};

#endif // !I_EVENT_MANAGER_H