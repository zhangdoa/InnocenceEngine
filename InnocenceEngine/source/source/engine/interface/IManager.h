#pragma once
#include "IObject.hpp"

#ifndef _I_MANAGER_H_
#define _I_MANAGER_H_


class IManager : public IObject
{
public:
	virtual ~IManager() {};

	const objectStatus& getStatus() const;
protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_ObjectStatus = objectStatus::SHUTDOWN;
};

#endif // !_I_MANAGER_H_