#pragma once
#include "interface/IGuiSystem.h"
#include "interface/ILogSystem.h"

extern ILogSystem* g_pLogSystem;

class GuiSystem : public IGuiSystem
{
public:
	GuiSystem() {};
	~GuiSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

