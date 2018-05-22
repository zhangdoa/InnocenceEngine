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
	void removeMesh(meshType meshType, meshID meshID) override;
	void removeTexture(textureID textureID) override;

	const objectStatus& getStatus() const override;

private:
	void setupWindow();
	void setupInput();
	void setupRendering();
	void setupCameraComponents();
	void setupCameraComponentProjectionMatrix(CameraComponent* cameraComponent);
	void setupCameraComponentRayOfEye(CameraComponent* cameraComponent);
	void setupCameraComponentFrustumVertices(CameraComponent* cameraComponent);

	void initializeWindow();
	void initializeInput();
	void initializeRendering();
	void loadDefaultAssets();
	void loadAssetsForComponents();


	void assignUnitMesh(VisibleComponent& visibleComponent, meshShapeType meshType);
	void assignLoadedTexture(textureAssignType textureAssignType, const texturePair& loadedTextureDataPair, VisibleComponent& visibleComponent);
	void assignDefaultTextures(textureAssignType textureAssignType, VisibleComponent & visibleComponent);
	void loadTexture(const std::vector<std::string>& fileName, textureType textureType, VisibleComponent& visibleComponent);
	void loadModel(const std::string& fileName, VisibleComponent& visibleComponent);
	void assignloadedModel(modelMap& loadedGraphicDataMap, VisibleComponent& visibleComponent);
	std::vector<Vertex> generateNDC();
	std::vector<Vertex> generateViewFrustum(const mat4& transformMatrix);
	void generateAABB(VisibleComponent & visibleComponent);
	void generateAABB(LightComponent & lightComponent);
	void generateAABB(CameraComponent & cameraComponent);
	AABB generateAABB(const std::vector<Vertex>& vertices);
	AABB generateAABB(const vec4& boundMax, const vec4& boundMin);
	meshID addMesh(const std::vector<Vertex>& vertices, const std::vector<unsigned int>& indices);
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
	std::multimap<int, std::vector<std::function<void()>*>> m_keyboardInputCallback;
	std::multimap<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;

	//Physic data
	Ray m_mouseRay;

	//Asset data
	std::unordered_map<meshID, BaseMesh*> m_meshMap;
	std::unordered_map<meshID, BaseMesh*> m_BBMeshMap;
	std::unordered_map<textureID, BaseTexture*> m_textureMap;
	std::unordered_map<textureID, BaseTexture*> m_shadowTextureMap;

	meshID m_UnitCubeTemplate;
	meshID m_UnitSphereTemplate;
	meshID m_Unit3DQuadTemplate;
	meshID m_Unit2DQuadTemplate;
	meshID m_UnitLineTemplate;
	textureID m_basicNormalTemplate;
	textureID m_basicAlbedoTemplate;
	textureID m_basicMetallicTemplate;
	textureID m_basicRoughnessTemplate;
	textureID m_basicAOTemplate;

	std::unordered_map<std::string, modelMap> m_loadedModelMap;
	std::unordered_map<std::string, texturePair> m_loadedTextureMap;

	//Rendering Data
	std::atomic<bool> m_canRender;
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;
	std::function<void()> f_changeShadingMode;

	std::vector<VisibleComponent*> m_staticMeshVisibleComponents;
	std::vector<VisibleComponent*> m_emissiveVisibleComponents;
	std::vector<VisibleComponent*> m_selectedVisibleComponents;
	std::vector<VisibleComponent*> m_inFrustumVisibleComponents;

	BaseFrameBuffer* m_environmentPassFrameBuffer;
	BaseShaderProgram* m_environmentCapturePassShaderProgram;
	BaseShaderProgram* m_environmentConvolutionPassShaderProgram;
	BaseShaderProgram* m_environmentPreFilterPassShaderProgram;
	BaseShaderProgram* m_environmentBRDFLUTPassShaderProgram;
	textureID m_environmentCapturePassTextureID;
	textureID m_environmentConvolutionPassTextureID;
	textureID m_environmentPreFilterPassTextureID;
	textureID m_environmentBRDFLUTTextureID;

	BaseFrameBuffer* m_shadowForwardPassFrameBuffer;
	BaseShaderProgram* m_shadowForwardPassShaderProgram;
	textureID m_shadowForwardPassTextureID_L0;
	textureID m_shadowForwardPassTextureID_L1;
	textureID m_shadowForwardPassTextureID_L2;
	textureID m_shadowForwardPassTextureID_L3;

	BaseFrameBuffer* m_geometryPassFrameBuffer;
	BaseShaderProgram* m_geometryPassShaderProgram;
	textureID m_geometryPassRT0TextureID;
	textureID m_geometryPassRT1TextureID;
	textureID m_geometryPassRT2TextureID;
	textureID m_geometryPassRT3TextureID;

	BaseFrameBuffer* m_lightPassFrameBuffer;
	BaseShaderProgram* m_lightPassShaderProgram;
	textureID m_lightPassTextureID;

	BaseFrameBuffer* m_skyPassFrameBuffer;
	BaseShaderProgram* m_skyPassShaderProgram;
	textureID m_skyPassTextureID;

	BaseFrameBuffer* m_debuggerPassFrameBuffer;
	BaseShaderProgram* m_debuggerPassShaderProgram;
	textureID m_debuggerPassTextureID;

	BaseFrameBuffer* m_billboardPassFrameBuffer;
	BaseShaderProgram* m_billboardPassShaderProgram;
	textureID m_billboardPassTextureID;

	BaseFrameBuffer* m_bloomExtractPassFrameBuffer;
	BaseShaderProgram* m_bloomExtractPassShaderProgram;
	textureID m_bloomExtractPassTextureID;

	BaseFrameBuffer* m_bloomBlurPassPingFrameBuffer;
	BaseFrameBuffer* m_bloomBlurPassPongFrameBuffer;
	BaseShaderProgram* m_bloomBlurPassShaderProgram;
	textureID m_bloomBlurPassPingTextureID;
	textureID m_bloomBlurPassPongTextureID;

	BaseFrameBuffer* m_finalPassFrameBuffer;
	BaseShaderProgram* m_finalPassShaderProgram;
	textureID m_finalPassTextureID;

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
