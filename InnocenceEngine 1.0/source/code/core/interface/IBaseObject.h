#pragma once

#ifndef _I_BASE_OBJECT_H_
#define _I_BASE_OBJECT_H_

enum class execMessage
{
	DEFAULT,
	INIT,
	UPDATE,
	SHUTDOWN
};

enum class objectStatus
{
	STANDBY,
	INITIALIZIED,
	UNINITIALIZIED,
	ERROR
};
class IBaseObject
{
public:
	IBaseObject();
	virtual ~IBaseObject();

	void exec(execMessage execMessage);
	const objectStatus& getStatus() const;

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_ObjectStatus = objectStatus::UNINITIALIZIED;

	virtual void init() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;
};

#endif // !_I_BASE_OBJECT_H_