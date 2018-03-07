#pragma once
#include "interface/IApplication.h"
#include "interface/ICoreManager.h"

extern ICoreManager* g_pCoreManager;

class BaseApplication : public IApplication
{
public:
	BaseApplication() {};
	~BaseApplication() {};

	virtual void setup() override;
	virtual void initialize() override;
	virtual void update() override;
	virtual void shutdown() override;

	const objectStatus& getStatus() const;

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

