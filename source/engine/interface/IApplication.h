#pragma once
#include "IObject.hpp"

class IApplication : public IObject
{
public:
	virtual ~IApplication() {};

	virtual const objectStatus& getStatus() const = 0;

protected:
	virtual void setStatus(objectStatus objectStatus) = 0;
};

