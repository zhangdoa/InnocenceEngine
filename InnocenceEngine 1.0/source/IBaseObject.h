#pragma once

#ifndef _I_BASE_OBJECT_H_
#define _I_BASE_OBJECT_H_

class IBaseObject
{
public:
	IBaseObject();
	virtual ~IBaseObject();

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
	managerStatus m_ObjectStatus = UNINITIALIZIED;

	virtual void init() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;

};

#endif // !_I_BASE_OBJECT_H_