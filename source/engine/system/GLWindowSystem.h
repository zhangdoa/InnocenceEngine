#pragma once
#include "interface/IWindowSystem.h"
#include "interface/ILogSystem.h"
#include "interface/IGameSystem.h"
#include "component/WindowSystemSingletonComponent.h"

extern ILogSystem* g_pLogSystem;
extern IGameSystem* g_pGameSystem;

class GLWindowSystem : public IWindowSystem
{
public:
	GLWindowSystem() {};
	~GLWindowSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	const objectStatus& getStatus() const override;

	vec4 calcMousePositionInWorldSpace() override;
	void swapBuffer() override;
private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback);
	void addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback);
	void addKeyboardInputCallback(std::unordered_map<int, std::vector<std::function<void()>*>>& keyboardInputCallback);
	void addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(std::unordered_map<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback);

	void framebufferSizeCallback(int width, int height);
	void mousePositionCallback(double mouseXPos, double mouseYPos);
	void scrollCallback(double xoffset, double yoffset);

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