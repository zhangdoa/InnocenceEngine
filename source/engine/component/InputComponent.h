#pragma once
#include "BaseComponent.h"

class InputComponent : public BaseComponent
{
public:
	InputComponent() {};
	~InputComponent() {};

	void registerKeyboardInputCallback(int keyCode, std::function<void()>* function);
	void registerMouseInputCallback(int mouseCode, std::function<void(double)>* function);
	
	std::unordered_map<int, std::vector<std::function<void()>*>>& getKeyboardInputCallbackContainer();
	std::unordered_map<int, std::vector<std::function<void(double)>*>>& getMouseInputCallbackContainer();

private:
	std::unordered_map<int, std::vector<std::function<void()>*>> m_keyboardInputCallbackImpl;
	std::unordered_map<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallbackImpl;
};
