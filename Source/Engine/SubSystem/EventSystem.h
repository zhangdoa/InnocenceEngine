#pragma once
#include "../Interface/IEventSystem.h"

namespace Inno
{
	class InnoEventSystem : public IEventSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(InnoEventSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		InputConfig getInputConfig() override;

		void addButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent) override;
		void addMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent) override;

		void buttonStateCallback(ButtonState buttonState) override;
		void windowSizeCallback(int32_t width, int32_t height) override;
		void mouseMovementCallback(float mouseXPos, float mouseYPos) override;
		void scrollCallback(float xoffset, float yoffset) override;

		Vec2 getMousePosition() override;
	};
}