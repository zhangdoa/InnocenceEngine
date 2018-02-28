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

class ICoreManager : public IManager
{
public:
	virtual ~ICoreManager() {};
};

class IRenderingManager : public IManager
{
public:
	virtual ~IRenderingManager() {};

	virtual void render() = 0;
};