#pragma once
#include "interface/IWindowSystem.h"
#include "interface/ILogSystem.h"
#include "interface/IGameSystem.h"
#include "component/WindowSystemSingletonComponent.h"

extern ILogSystem* g_pLogSystem;
extern IGameSystem* g_pGameSystem;

class BaseWindowSystem : public IWindowSystem
{
public:
	BaseWindowSystem() {};
	~BaseWindowSystem() {};

	vec4 calcMousePositionInWorldSpace() override;
	void swapBuffer() override;

protected:
	void addInputCallback();
	void addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback);
	void addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback);
	void addKeyboardInputCallback(std::unordered_map<int, std::vector<std::function<void()>*>>& keyboardInputCallback);
	void addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(std::unordered_map<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback);

	void framebufferSizeCallback(int width, int height);
	void mousePositionCallback(double mouseXPos, double mouseYPos);
	void scrollCallback(double xoffset, double yoffset);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};