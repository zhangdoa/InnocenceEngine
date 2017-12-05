#pragma once

#ifndef _I_BASE_OBJECT_H_
#define _I_BASE_OBJECT_H_

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

	const objectStatus& getStatus() const;
	virtual void initialize() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_ObjectStatus = objectStatus::SHUTDOWN;
};

#endif // !_I_BASE_OBJECT_H_