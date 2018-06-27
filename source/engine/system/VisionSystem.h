#pragma once
#include<atomic>

#include "common/GLHeaders.h"

#include "third-party/ImGui/imgui.h"
#include "third-party/ImGui/imgui_impl_glfw_gl3.h"

#include "interface/IVisionSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "interface/IGameSystem.h"
#include "interface/IPhysicsSystem.h"

#include "common/ComponentHeaders.h"
#include "GLRenderingSystem.h"

extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;
extern IGameSystem* g_pGameSystem;
extern IPhysicsSystem* g_pPhysicsSystem;

enum class keyPressType { CONTINUOUS, ONCE };

class keyButton
{
public:
	keyButton() {};
	~keyButton() {};

	keyPressType m_keyPressType = keyPressType::CONTINUOUS;
	bool m_allowCallback = true;
};

class VisionSystem : public IVisionSystem
{
public:
	VisionSystem() {};
	~VisionSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	friend class windowCallbackWrapper;

	void setWindowName(const std::string& windowName) override;

	const objectStatus& getStatus() const override;

private:
	void setupWindow();
	void setupInput();
	void setupRendering();
	void setupGui();

	void initializeWindow();
	void initializeInput();
	void initializeRendering();
	void initializeGui();

	void addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback);
	void addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback);
	void addKeyboardInputCallback(std::unordered_map<int, std::vector<std::function<void()>*>>& keyboardInputCallback);
	void addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(std::unordered_map<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback);
	void framebufferSizeCallback(int width, int height);
	void mousePositionCallback(double mouseXPos, double mouseYPos);
	void scrollCallback(double xoffset, double yoffset);

	vec4 calcMousePositionInWorldSpace();

	void updateInput();
	void updateRendering();
	void updateGui();
	
	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void changeShadingMode();
	void hideMouseCursor() const;
	void showMouseCursor() const;

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	//Window data
	vec2 m_screenResolution = vec2(1280, 720);
	GLFWwindow* m_window;
	std::string m_windowName;

	//Input data
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	std::unordered_map<int, keyButton> m_keyButtonMap;
	std::unordered_map<int, std::vector<std::function<void()>*>> m_keyboardInputCallback;
	std::unordered_map<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;

	//Physic data
	Ray m_mouseRay;

	//Rendering Data
	std::atomic<bool> m_canRender;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;
	std::function<void()> f_changeShadingMode;

	IRenderingSystem* m_RenderingSystem;

	int m_polygonMode = 2;
	int m_textureMode = 0;
	int m_shadingMode = 0;
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

	void setVisionSystem(VisionSystem* visionSystem);
	void initialize();
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
	windowCallbackWrapper() {};

	VisionSystem* m_visionSystem;

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);
};
