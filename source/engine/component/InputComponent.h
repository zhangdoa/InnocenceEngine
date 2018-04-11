#pragma once
#include "BaseComponent.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent();
	~InputComponent();


	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	void registerInputCallback(int keyCode, void* function);
	std::multimap<int, std::vector<std::function<void()>*>>& getKeyboardInputCallbackImpl();
	std::multimap<int, std::vector<std::function<void(double)>*>>& getMouseInputCallbackImpl();

private:
	enum moveDirection { FORWARD, BACKWARD, LEFT, RIGHT };

	void moveForward ();
	void moveBackward ();
	void moveLeft ();
	void moveRight ();

	void move(moveDirection moveDirection);

	void rotateAroundXAxis(double offset);
	void rotateAroundYAxis(double offset);

	std::multimap<int, std::vector<std::function<void()>*>> m_keyboardInputCallbackImpl;
	std::multimap<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallbackImpl;

	std::function<void()> f_moveForward;
	std::function<void()> f_moveBackward;
	std::function<void()> f_moveLeft;
	std::function<void()> f_moveRight;
	std::function<void(double)> f_rotateAroundXAxis;
	std::function<void(double)> f_rotateAroundYAxis;

	double moveSpeed = 0.5f;
	double rotateSpeed = 2.0f;

};

