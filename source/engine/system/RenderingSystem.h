#pragma once
#include<atomic>

#include "common/GLHeaders.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IAssetSystem.h"
#include "interface/IGameSystem.h"
#include "interface/ILogSystem.h"

#include "entity/ComponentHeaders.h"
#include "entity/BaseGraphicPrimitiveHeader.h"
#include "entity/GLGraphicPrimitiveHeader.h"

extern IMemorySystem* g_pMemorySystem;
extern ILogSystem* g_pLogSystem;
extern IAssetSystem* g_pAssetSystem;
extern IGameSystem* g_pGameSystem;

enum class keyPressType { CONTINUOUS, ONCE };

class keyButton
{
public:
	keyButton() {};
	~keyButton() {};

	keyPressType m_keyPressType = keyPressType::CONTINUOUS;
	bool m_allowCallback = true;
};

class RenderingSystem : public IRenderingSystem
{
public:
	RenderingSystem() {};
	~RenderingSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;
	bool canRender() override;

	friend class windowCallbackWrapper;

	void setWindowName(const std::string& windowName) override;

	void render() override;

	const objectStatus& getStatus() const override;

private:
	void setupWindow();
	void setupInput();
	void setupRendering();

	void initializeWindow();
	void initializeInput();
	void initializeRendering();

	void setupComponents();
	void setupCameraComponents();
	void setupCameraComponentProjectionMatrix(CameraComponent* cameraComponent);
	void setupCameraComponentRayOfEye(CameraComponent* cameraComponent);
	void setupCameraComponentFrustumVertices(CameraComponent* cameraComponent);
	void setupVisibleComponents();
	void setupLightComponents();
	void setupLightComponentRadius(LightComponent* lightComponent);

	void addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback);
	void addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback);
	void addKeyboardInputCallback(std::unordered_map<int, std::vector<std::function<void()>*>>& keyboardInputCallback);
	void addMouseMovementCallback(int mouseCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int mouseCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(std::unordered_map<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback);

	std::vector<Vertex> generateNDC();
	std::vector<Vertex> generateViewFrustum(const mat4& transformMatrix);
	void generateAABB(VisibleComponent & visibleComponent);
	void generateAABB(LightComponent & lightComponent);
	void generateAABB(CameraComponent & cameraComponent);
	AABB generateAABB(const std::vector<Vertex>& vertices);
	AABB generateAABB(const vec4& boundMax, const vec4& boundMin);
	vec4 calcMousePositionInWorldSpace();

	void updateInput();
	void updatePhysics();
	void updateCameraComponents();

	void initializeBackgroundPass();
	void renderBackgroundPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeShadowPass();
	void renderShadowPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeGeometryPass();
	void renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeLightPass();
	void renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeFinalPass();
	void renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
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

	std::vector<VisibleComponent*> m_staticMeshVisibleComponents;
	std::vector<VisibleComponent*> m_emissiveVisibleComponents;
	std::vector<VisibleComponent*> m_selectedVisibleComponents;
	std::vector<VisibleComponent*> m_inFrustumVisibleComponents;

	int m_polygonMode = 2;
	int m_textureMode = 0;
	int m_shadingMode = 0;

	bool m_shouldUpdateEnvironmentMap = true;
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

	void setRenderingSystem(RenderingSystem* RenderingSystem);
	void initialize();
	static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
	static void mousePositionCallback(GLFWwindow* window, double mouseXPos, double mouseYPos);
	static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
	windowCallbackWrapper() {};

	RenderingSystem* m_renderingSystem;

	void framebufferSizeCallbackImpl(GLFWwindow* window, int width, int height);
	void mousePositionCallbackImpl(GLFWwindow* window, double mouseXPos, double mouseYPos);
	void scrollCallbackImpl(GLFWwindow* window, double xoffset, double yoffset);
};
