#pragma once
#include "BaseWindowSystem.h"
#include "interface/ILogSystem.h"
#include "interface/IGameSystem.h"
#include "component/WindowSystemSingletonComponent.h"

extern ILogSystem* g_pLogSystem;
extern IGameSystem* g_pGameSystem;

class GLWindowSystem : public BaseWindowSystem
{
public:
	GLWindowSystem() {};
	~GLWindowSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	void swapBuffer() override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void hideMouseCursor() const;
	void showMouseCursor() const;

	friend class windowCallbackWrapper;
};

class windowCallbackWrapper
{
public:
	~windowCallbackWrapper() {};

	static windowCallbackWrapper& getInstance()
	{
		static windowCallbackWrapper instance;
		return instance;
	}

	void setWindowSystem(GLWindowSystem* GLWindowSystem);
	void initialize();

	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);

private:
	windowCallbackWrapper() {};

	GLWindowSystem* m_GLWindowSystem;
};