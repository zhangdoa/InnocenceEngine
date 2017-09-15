#pragma once

#ifndef _I_BASE_OBJECT_H_
#define _I_BASE_OBJECT_H_

enum class executeMessage
{
	INITIALIZE,
	UPDATE,
	SHUTDOWN
};

enum class objectStatus
{
	STANDBY,
	ALIVE,
	SHUTDOWN,
	ERROR
};
class IBaseObject
{
public:
	IBaseObject();
	virtual ~IBaseObject();

	void excute(executeMessage execteMessage);
	const objectStatus& getStatus() const;

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_ObjectStatus = objectStatus::SHUTDOWN;

	virtual void initialize() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;
};

#endif // !_I_BASE_OBJECT_H_