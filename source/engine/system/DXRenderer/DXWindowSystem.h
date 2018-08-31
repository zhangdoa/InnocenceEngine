#pragma once

#include "../BaseWindowSystem.h"
#include "../../interface/ILogSystem.h"
#include "../../interface/IGameSystem.h"
#include "../../component/WindowSystemSingletonComponent.h"

extern ILogSystem* g_pLogSystem;
extern IGameSystem* g_pGameSystem;

class DXWindowSystem : public BaseWindowSystem
{
public:
	DXWindowSystem() {};
	~DXWindowSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	void swapBuffer() override;

	LRESULT CALLBACK MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

static DXWindowSystem* ApplicationHandle = 0;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);