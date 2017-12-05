#pragma once
#include "../interface/IGameEntity.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent();
	~InputComponent();

	void initialize() override;
	void update() override;
	void shutdown() override;

	void registerInputCallback(int keyCode, void* function);
	std::multimap<int, std::function<void()>>& getKeyboardInputCallbackImpl();
	std::multimap<int, std::function<void(float)>>& getMouseInputCallbackImpl();

private:
	enum moveDirection { FORWARD, BACKWARD, LEFT, RIGHT };

	void moveForward ();
	void moveBackward ();
	void moveLeft ();
	void moveRight ();

	void move(moveDirection moveDirection);

	void rotateAroundXAxis(float offset);
	void rotateAroundYAxis(float offset);

	std::multimap<int, std::function<void()>> m_keyboardInputCallbackImpl;
	std::multimap<int, std::function<void(float)>> m_mouseMovementCallbackImpl;

	float moveSpeed = 0.05f;
	float rotateSpeed = 2.0f;

};

