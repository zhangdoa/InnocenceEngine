#pragma once

#ifndef _I_BASE_OBJECT_H_
#define _I_BASE_OBJECT_H_

enum class objectStatus
{
	STANDBY,
	ALIVE,
	SHUTDOWN,
};

class IObject
{
public:
	IObject();
	virtual ~IObject();

	const objectStatus& getStatus() const;

	// setup() only sets static member data
	virtual void setup() = 0;
	// initialize() only excutes dynamic function invocation
	virtual void initialize() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_ObjectStatus = objectStatus::SHUTDOWN;
};

#endif // !_I_BASE_OBJECT_H_