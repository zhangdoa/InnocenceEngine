#pragma once
#include "IObject.hpp"

class IApplication : public IObject
{
public:
	virtual ~IApplication() {};

	const objectStatus& getStatus() const;
protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

