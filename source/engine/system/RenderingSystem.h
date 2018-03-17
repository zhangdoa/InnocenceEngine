#pragma once

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IAssetSystem.h"
#include "interface/IGameSystem.h"
#include "interface/ILogSystem.h"

#include "entity/ComponentHeaders.h"
#include "entity/GLGraphicPrimitive.h"
#include "entity/GLShader.h"

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

	void addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback);
	void addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback);
	void addKeyboardInputCallback(std::multimap<int, std::vector<std::function<void()>*>>& keyboardInputCallback);
	void addMouseMovementCallback(int keyCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int keyCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(std::multimap<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback);

	void setWindowName(const std::string& windowName) override;

	void render() override;
	meshID addMesh(meshType meshType) override;
	textureID addTexture(textureType textureType) override;
	BaseMesh* getMesh(meshType meshType, meshID meshID) override;
	BaseTexture* getTexture(textureType textureType, textureID textureID) override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	//Window data
	unsigned int SCR_WIDTH = 1280;
	unsigned int SCR_HEIGHT = 720;
	vec2 m_screenResolution = vec2(SCR_WIDTH, SCR_HEIGHT);
	GLFWwindow* m_window;
	std::string m_windowName;

	GLFWwindow* getWindow() const;
	vec2 getScreenCenterPosition() const;
	vec2 getScreenResolution() const;
	void hideMouseCursor() const;
	void showMouseCursor() const;

	//Input data
	const int NUM_KEYCODES = 256;
	const int NUM_MOUSEBUTTONS = 5;

	std::unordered_map<int, keyButton> m_keyButtonMap;
	std::multimap<int, std::vector<std::function<void()>*>> m_keyboardInputCallback;
	std::multimap<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;

	//Asset data
	enum class textureAssignType { ADD_DEFAULT, OVERWRITE };

	std::unordered_map<meshID, GL2DMesh> m_2DMeshMap;
	std::unordered_map<meshID, GL3DMesh> m_3DMeshMap;
	std::unordered_map<textureID, GL2DTexture> m_2DTextureMap;
	std::unordered_map<textureID, GL2DHDRTexture> m_2DHDRTextureMap;
	std::unordered_map<textureID, GL3DTexture> m_3DTextureMap;
	std::unordered_map<textureID, GL3DHDRTexture> m_3DHDRTextureMap;

	void assignUnitMesh(VisibleComponent& visibleComponent, meshShapeType meshType);
	void assignLoadedTexture(textureAssignType textureAssignType, texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);
	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void loadTexture(const std::vector<std::string>& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	void assignloadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);

	meshID m_UnitCubeTemplate;
	meshID m_UnitSphereTemplate;
	meshID m_UnitQuadTemplate;
	textureID m_basicNormalTemplate;
	textureID m_basicAlbedoTemplate;
	textureID m_basicMetallicTemplate;
	textureID m_basicRoughnessTemplate;
	textureID m_basicAOTemplate;

	std::unordered_map<std::string, modelMap> m_loadedModelMap;
	std::unordered_map<std::string, texturePair> m_loadedTextureMap;

	//Rendering Data
	std::atomic<bool> m_canRender = true;
	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	void changeShadingMode();
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;
	std::function<void()> f_changeShadingMode;

	GLFrameBuffer m_geometryPassFrameBuffer;
	GLShaderProgram* m_geometryPassShader;

	GLFrameBuffer m_lightPassFrameBuffer;
	GLShaderProgram* m_lightPassShader;

	GLFrameBuffer m_environmentPassFrameBuffer;
	GLuint m_environmentPassFBO;
	GLuint m_environmentPassRBO;
	textureID m_environmentCapturePassTextureID;
	textureID m_environmentConvolutionPassTextureID;
	textureID m_environmentPreFilterPassTextureID;

	textureID m_environmentBRDFLUTTextureID;
	GLuint m_environmentBRDFLUTTexture;
	BaseMesh* m_environmentBRDFLUTMesh;

	GLShaderProgram* m_environmentCapturePassShader;
	GLShaderProgram* m_environmentConvolutionPassShader;
	GLShaderProgram* m_environmentPreFilterPassShader;
	GLShaderProgram* m_environmentBRDFLUTPassShader;

	GLFrameBuffer m_skyForwardPassFrameBuffer;
	GLShaderProgram* m_skyForwardPassShader;

	GLFrameBuffer m_skyDeferPassFrameBuffer;
	GLShaderProgram* m_skyDeferPassShader;

	GLFrameBuffer m_debuggerPassFrameBuffer;
	GLShaderProgram* m_debuggerPassShader;

	BaseMesh* m_finalPassMesh;
	GLFrameBuffer m_finalPassFrameBuffer;
	GLShaderProgram* m_finalPassShader;

	int m_polygonMode = 0;
	int m_textureMode = 0;
	int m_shadingMode = 0;

	bool m_shouldUpdateEnvironmentMap = true;
	void initializeGeometryPass();
	void renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeLightPass();
	void renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeBackgroundPass();
	void renderBackgroundPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeFinalPass();
	void renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
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
