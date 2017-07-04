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
		DEFAULT,
		INIT,
		UPDATE,
		SHUTDOWN	
	};

	enum managerStatus
	{
		STANDBY,
		INITIALIZIED,
		UNINITIALIZIED,
		ERROR
	};


	void exec(execMessage execMessage);
	const int getStatus() const;

protected:
	void setStatus(managerStatus managerStatus);
	static void printLog(std::string logMessage);

private:
	managerStatus m_managerStatus = UNINITIALIZIED;

	virtual void init() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;

};

#endif // !I_EVENT_MANAGER_H