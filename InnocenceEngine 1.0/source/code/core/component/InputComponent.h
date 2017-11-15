#pragma once
#include "../interface/IGameEntity.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent();
	~InputComponent();

	void registerInputCallback(int keyCode, void* function);
	std::multimap<int, std::function<void()>>& getKeyboardInputCallbackImpl();
	std::multimap<int, std::function<void(float)>>& getMouseInputCallbackImpl();

private:
	enum moveDirection { FORWARD, BACKWARD, LEFT, RIGHT };

	void initialize() override;
	void update() override;
	void shutdown() override;

	void moveForward ();
	void moveBackward ();
	void moveLeft ();
	void moveRight ();

	void move(moveDirection moveDirection);

	void rotateAroundXAxis(float offset);
	void rotateAroundYAxis(float offset);

	glm::vec2 l_mousePosition;
	int l_mouseRightResult = 0;

	int l_ketEscapeResult = 0;
	int l_keyF1Result = 0;
	int l_oldKeyF1Result = 0;
	int l_keyF2Result = 0;
	int l_oldKeyF2Result = 0;

	std::multimap<int, std::function<void()>> m_keyboardInputCallbackImpl;
	std::multimap<int, std::function<void(float)>> m_mouseMovementCallbackImpl;

	float moveSpeed = 0.05f;
	float rotateSpeed = 2.0f;

};

