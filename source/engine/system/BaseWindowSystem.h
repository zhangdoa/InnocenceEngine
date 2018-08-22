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

	void setup() override;
	void initialize() override;
	void update() override;

	vec4 calcMousePositionInWorldSpace() override;
	void swapBuffer() override;

protected:
	void addButtonStatusCallback(button boundButton, std::function<void()>* buttonStatusCallbackFunctor);
	void addButtonStatusCallback(button boundButton, std::vector<std::function<void()>*>& buttonStatusCallbackFunctor);
	void addButtonStatusCallback(buttonStatusCallbackMap & buttonStatusCallbackFunctor);
	void addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(mouseMovementCallbackMap& mouseMovementCallback);

	void framebufferSizeCallback(int width, int height);
	void mousePositionCallback(double mouseXPos, double mouseYPos);
	void scrollCallback(double xoffset, double yoffset);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};