#pragma once
#include "IObject.hpp"

class IManager : public IObject
{
public:
	virtual ~IManager() {};

	virtual const objectStatus& getStatus() const = 0;

protected:
	virtual void setStatus(objectStatus objectStatus) = 0;
};