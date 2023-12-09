#pragma once
#include "../Interface/IEventSystem.h"

namespace Inno
{
	class EventSystemImpl;
	class EventSystem : public IEventSystem
	{
	public:
		INNO_CLASS_CONCRETE_NON_COPYABLE(EventSystem);

		bool Setup(ISystemConfig* systemConfig) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		InputConfig GetInputConfig() override;

		void AddButtonStateCallback(ButtonState buttonState, ButtonEvent buttonEvent) override;
		void AddMouseMovementCallback(MouseMovementAxis mouseMovementAxis, MouseMovementEvent mouseMovementEvent) override;

		void ButtonStateCallback(ButtonState buttonState) override;
		void WindowResizeCallback(int32_t width, int32_t height) override;
		void MouseMovementCallback(float mouseXPos, float mouseYPos) override;
		void ScrollCallback(float xOffset, float yOffset) override;

		Vec2 GetMousePosition() override;

	private:
		EventSystemImpl* m_Impl;
	};
}