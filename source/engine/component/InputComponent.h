#pragma once
#include "BaseComponent.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent();
	~InputComponent();


	void setup() override;
	void initialize() override;
	void shutdown() override;

	template<typename T>
	void registerInputCallback(int keyCode, void* function, T* owner);


	std::multimap<int, std::vector<std::function<void()>*>>& getKeyboardInputCallbackImpl();
	std::multimap<int, std::vector<std::function<void(double)>*>>& getMouseInputCallbackImpl();

private:
	void rotateAroundPositiveYAxis(double offset);
	void rotateAroundRightAxis(double offset);

	std::multimap<int, std::vector<std::function<void()>*>> m_keyboardInputCallbackImpl;
	std::multimap<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallbackImpl;

	std::function<void(double)> f_rotateAroundPositiveYAxis;
	std::function<void(double)> f_rotateAroundRightAxis;

	double moveSpeed = 0.5f;
	double rotateSpeed = 2.0f;
};
