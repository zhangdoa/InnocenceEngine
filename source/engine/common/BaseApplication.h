#pragma once
#include "interface/IApplication.h"
#include "interface/ICoreSystem.h"

#if defined(INNO_RENDERER_DX)
#include <windows.h>
#endif

extern ICoreSystem* g_pCoreSystem;

class BaseApplication : public IApplication
{
public:
	BaseApplication() {};
	~BaseApplication() {};

#if defined(INNO_RENDERER_DX)
	void setup() override {};
	void setup(void* appInstance, char* commandLineArg, int showMethod) override;
#else
	void setup() override;
#endif
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

BaseApplication g_App;
IApplication* g_pApp = &g_App;