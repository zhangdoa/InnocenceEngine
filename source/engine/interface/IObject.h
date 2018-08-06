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
	virtual const objectStatus& getStatus() const = 0;
};

#endif // !_I_BASE_OBJECT_HPP_