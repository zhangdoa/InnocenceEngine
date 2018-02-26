#pragma once
#include "common/stdafx.h"
#include "interface/IManager.h"

class BaseManager : public IManager
{
public:
	~BaseManager() {};

	const objectStatus& getStatus() const override;

protected:
	void setStatus(objectStatus objectStatus) override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

