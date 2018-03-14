#pragma once

#include "interface/IRenderingSystem.h"
#include "interface/IAssetSystem.h"
#include "interface/IGameSystem.h"
#include "interface/ILogSystem.h"

#include "entity/ComponentHeaders.h"
#include "entity/GLGraphicPrimitive.h"

extern ILogSystem* g_pLogSystem;
extern IAssetSystem* g_pAssetSystem;
extern IGameSystem* g_pGameSystem;

class GLShader
{
public:
	virtual ~GLShader();

	virtual void init() = 0;

	virtual void shaderDraw() {};

	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap) {};
	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture& threeDTexture) {};
	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) {};

	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents) {};
	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) {};

	virtual void shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DHDRTexture>& twoDTextureMap, GL3DHDRTexture& threeDTexture) {};
	virtual void shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture& threeDCapturedTexture, GL3DHDRTexture& threeDConvolutedTexture) {};


protected:
	GLShader();

	enum shaderType
	{
		VERTEX,
		GEOMETRY,
		FRAGMENT
	};

	inline void addShader(shaderType shaderType, const std::string& fileLocation) const;
	inline void setAttributeLocation(int arrtributeLocation, const std::string& arrtributeName) const;
	inline void bindShader() const;

	inline void initProgram();
	inline void addUniform(std::string uniform) const;
	inline GLint getUniformLocation(const std::string &uniformName) const;
	inline void updateUniform(const GLint uniformLocation, bool uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, int uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z, double w);
	inline void updateUniform(const GLint uniformLocation, const mat4& mat) const;

private:
	inline void attachShader(shaderType shaderType, const std::string& fileContent, int m_program) const;
	inline void compileShader() const;
	inline void detachShader(int shader) const;

	unsigned int m_program;
};

class GeometryPassBlinnPhongShader : public GLShader
{
public:
	~GeometryPassBlinnPhongShader() {};

	static GeometryPassBlinnPhongShader& getInstance()
	{
		static GeometryPassBlinnPhongShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	GeometryPassBlinnPhongShader() {};

	GLint m_uni_normalTexture;
	GLint m_uni_diffuseTexture;
	GLint m_uni_specularTexture;
	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;
};

class LightPassBlinnPhongShader : public GLShader
{
public:
	~LightPassBlinnPhongShader() {};

	static LightPassBlinnPhongShader& getInstance()
	{
		static LightPassBlinnPhongShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents) override;

private:
	LightPassBlinnPhongShader() {};

	GLint m_uni_RT0;
	GLint m_uni_RT1;
	GLint m_uni_RT2;
	GLint m_uni_RT3;

	GLint m_uni_viewPos;
	GLint m_uni_dirLight_direction;
	GLint m_uni_dirLight_color;

	std::vector<GLint> m_uni_pointLights_position;
	std::vector<GLint> m_uni_pointLights_radius;
	std::vector<GLint> m_uni_pointLights_color;

	bool isPointLightUniformAdded = false;
};

class GeometryPassPBSShader : public GLShader
{
public:
	~GeometryPassPBSShader() {};

	static GeometryPassPBSShader& getInstance()
	{
		static GeometryPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	GeometryPassPBSShader() {};

	GLint m_uni_normalTexture;
	GLint m_uni_albedoTexture;
	GLint m_uni_metallicTexture;
	GLint m_uni_roughnessTexture;
	GLint m_uni_aoTexture;

	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;

	GLint m_uni_useTexture;
	GLint m_uni_albedo;
	GLint m_uni_MRA;
};

class LightPassPBSShader : public GLShader
{
public:
	~LightPassPBSShader() {};

	static LightPassPBSShader& getInstance()
	{
		static LightPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents) override;

private:
	LightPassPBSShader() {};

	GLint m_uni_geometryPassRT0;
	GLint m_uni_geometryPassRT1;
	GLint m_uni_geometryPassRT2;
	GLint m_uni_geometryPassRT3;
	GLint m_uni_irradianceMap;
	GLint m_uni_preFiltedMap;
	GLint m_uni_brdfLUT;

	GLint m_uni_viewPos;
	GLint m_uni_dirLight_direction;
	GLint m_uni_dirLight_color;

	std::vector<GLint> m_uni_pointLights_position;
	std::vector<GLint> m_uni_pointLights_radius;
	std::vector<GLint> m_uni_pointLights_color;

	bool isPointLightUniformAdded = false;

};

class EnvironmentCapturePassPBSShader : public GLShader
{
public:
	~EnvironmentCapturePassPBSShader() {};

	static EnvironmentCapturePassPBSShader& getInstance()
	{
		static EnvironmentCapturePassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DHDRTexture>& twoDTextureMap, GL3DHDRTexture& threeDTexture);

private:
	EnvironmentCapturePassPBSShader() {};

	GLint m_uni_equirectangularMap;

	GLint m_uni_p;
	GLint m_uni_r;
};

class EnvironmentConvolutionPassPBSShader : public GLShader
{
public:
	~EnvironmentConvolutionPassPBSShader() {};

	static EnvironmentConvolutionPassPBSShader& getInstance()
	{
		static EnvironmentConvolutionPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture& threeDCapturedTexture, GL3DHDRTexture& threeDConvolutedTexture);

private:
	EnvironmentConvolutionPassPBSShader() {};

	GLint m_uni_capturedCubeMap;

	GLint m_uni_p;
	GLint m_uni_r;
};

class EnvironmentPreFilterPassPBSShader : public GLShader
{
public:
	~EnvironmentPreFilterPassPBSShader() {};

	static EnvironmentPreFilterPassPBSShader& getInstance()
	{
		static EnvironmentPreFilterPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture& threeDCapturedTexture, GL3DHDRTexture& threeDPreFiltedTexture);

private:
	EnvironmentPreFilterPassPBSShader() {};

	GLint m_uni_capturedCubeMap;

	GLint m_uni_p;
	GLint m_uni_r;

	GLint m_uni_roughness;
};

class EnvironmentBRDFLUTPassPBSShader : public GLShader
{
public:
	~EnvironmentBRDFLUTPassPBSShader() {};

	static EnvironmentBRDFLUTPassPBSShader& getInstance()
	{
		static EnvironmentBRDFLUTPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw();

private:
	EnvironmentBRDFLUTPassPBSShader() {};
};

class SkyForwardPassPBSShader : public GLShader
{
public:
	~SkyForwardPassPBSShader() {};

	static SkyForwardPassPBSShader& getInstance()
	{
		static SkyForwardPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, GL3DHDRTexture& threeDTexture);

private:
	SkyForwardPassPBSShader() {};

	GLint m_uni_skybox;

	GLint m_uni_p;
	GLint m_uni_r;
};

class DebuggerShader : public GLShader
{
public:
	~DebuggerShader() {};

	static DebuggerShader& getInstance()
	{
		static DebuggerShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap) override;

private:
	DebuggerShader() {};

	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;
};

class SkyDeferPassPBSShader : public GLShader
{
public:
	~SkyDeferPassPBSShader() {};

	static SkyDeferPassPBSShader& getInstance()
	{
		static SkyDeferPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw();

private:
	SkyDeferPassPBSShader() {};

	GLint m_uni_lightPassRT0;
	GLint m_uni_skyForwardPassRT0;
	GLint m_uni_debuggerPassRT0;
};

class FinalPassShader : public GLShader
{
public:
	~FinalPassShader() {};

	static FinalPassShader& getInstance()
	{
		static FinalPassShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw() override;

private:
	FinalPassShader() {};

	GLint m_uni_skyDeferPassRT0;
};

class BillboardPassShader : public GLShader
{
public:
	~BillboardPassShader() {};

	static BillboardPassShader& getInstance()
	{
		static BillboardPassShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	BillboardPassShader() {};
	GLint m_uni_texture;
	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;
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

	friend class windowCallbackWrapper;

	void addKeyboardInputCallback(int keyCode, std::function<void()>* keyboardInputCallback);
	void addKeyboardInputCallback(int keyCode, std::vector<std::function<void()>*>& keyboardInputCallback);
	void addKeyboardInputCallback(std::multimap<int, std::vector<std::function<void()>*>>& keyboardInputCallback);
	void addMouseMovementCallback(int keyCode, std::function<void(double)>* mouseMovementCallback);
	void addMouseMovementCallback(int keyCode, std::vector<std::function<void(double)>*>& mouseMovementCallback);
	void addMouseMovementCallback(std::multimap<int, std::vector<std::function<void(double)>*>>& mouseMovementCallback);

	void setWindowName(const std::string& windowName) override;

	void render() override;
	meshID addMesh() override;
	textureID addTexture(textureType textureType) override;
	BaseMesh* getMesh(meshID meshID) override;
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

	std::multimap<int, std::vector<std::function<void()>*>> m_keyboardInputCallback;
	std::multimap<int, std::vector<std::function<void(double)>*>> m_mouseMovementCallback;

	double m_mouseXOffset;
	double m_mouseYOffset;
	double m_mouseLastX;
	double m_mouseLastY;

	//Asset data
	enum class textureAssignType { ADD_DEFAULT, OVERWRITE };

	std::unordered_map<meshID, GLMesh> m_meshMap;
	std::unordered_map<textureID, GL2DTexture> m_2DTextureMap;
	std::unordered_map<textureID, GL2DHDRTexture> m_2DHDRTextureMap;
	std::unordered_map<textureID, GL3DTexture> m_3DTextureMap;
	std::unordered_map<textureID, GL3DHDRTexture> m_3DHDRTextureMap;

	void assignUnitMesh(VisibleComponent& visibleComponent, meshType meshType);
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
	void changeDrawPolygonMode();
	void changeDrawTextureMode();
	std::function<void()> f_changeDrawPolygonMode;
	std::function<void()> f_changeDrawTextureMode;

	GLFrameBuffer m_geometryPassFrameBuffer;
	GLShader* m_geometryPassShader;

	GLFrameBuffer m_lightPassFrameBuffer;
	GLShader* m_lightPassShader;

	GLuint m_environmentPassFBO;
	GLuint m_environmentPassRBO;
	textureID m_environmentCapturePassTextureID;
	textureID m_environmentConvolutionPassTextureID;
	textureID m_environmentPreFilterPassTextureID;
	GLuint m_environmentBRDFLUTTexture;
	GLuint m_environmentBRDFLUTPassVAO;
	GLuint m_environmentBRDFLUTPassVBO;
	std::vector<float> m_environmentBRDFLUTPassVertices;

	GLShader* m_environmentCapturePassShader;
	GLShader* m_environmentConvolutionPassShader;
	GLShader* m_environmentPreFilterPassShader;
	GLShader* m_environmentBRDFLUTPassShader;

	GLFrameBuffer m_skyForwardPassFrameBuffer;
	GLShader* m_skyForwardPassShader;

	GLFrameBuffer m_skyDeferPassFrameBuffer;
	GLShader* m_skyDeferPassShader;

	GLFrameBuffer m_debuggerPassFrameBuffer;
	GLShader* m_debuggerPassShader;

	GLuint m_finalPassVAO;
	GLuint m_finalPassVBO;
	std::vector<float> m_screenVertices;
	GLFrameBuffer m_finalPassFrameBuffer;
	GLShader* m_finalPassShader;

	int m_polygonMode = 0;
	int m_textureMode = 0;
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