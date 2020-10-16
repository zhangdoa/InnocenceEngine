#pragma once
#include "../../Interface/IWindowSystem.h"

class LinuxWindowSystem : public IWindowSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(LinuxWindowSystem);

	bool Setup(void* hInstance, void* hwnd) override;
	bool Initialize() override;
	bool Update() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	IWindowSurface* getWindowSurface() override;
	const std::vector<ButtonState>& getButtonState() override;

	bool sendEvent(uint32_t umsg, uint32_t WParam, int32_t LParam) override;
	bool addEventCallback(WindowEventCallbackFunctor* functor) override;
};
