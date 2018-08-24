#pragma once
#include "interface/IApplication.h"
#include "interface/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

class BaseApplication : public IApplication
{
public:
	BaseApplication() {};
	~BaseApplication() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

BaseApplication g_App;
IApplication* g_pApp = &g_App;