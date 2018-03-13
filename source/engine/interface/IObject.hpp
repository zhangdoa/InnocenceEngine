#pragma once

#ifndef _I_BASE_OBJECT_HPP_
#define _I_BASE_OBJECT_HPP_

enum class objectStatus
{
	STANDBY,
	ALIVE,
	SHUTDOWN,
};

class IObject
{
public:
	virtual ~IObject() {};

	// setup() only sets static member data
	virtual void setup() = 0;
	// initialize() only excutes dynamic function invocation
	virtual void initialize() = 0;
	virtual void update() = 0;
	virtual void shutdown() = 0;

	virtual const objectStatus& getStatus() const = 0;
};

#endif // !_I_BASE_OBJECT_HPP_