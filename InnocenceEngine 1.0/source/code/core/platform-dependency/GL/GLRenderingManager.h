#pragma once
#include "../../interface/IManager.h"
#include "../../manager/AssetManager.h"
#include "../../manager/LogManager.h"
#include "../../component/VisibleComponent.h"
#include "../../component/LightComponent.h"
#include "../../component/CameraComponent.h"
class GLShader
{
public:
	virtual ~GLShader();

	virtual void init() = 0;
	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) {};
	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL3DTexture>& textureMap) {};

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

	inline void updateUniform(const std::string &uniformName, bool uniformValue) const;
	inline void updateUniform(const std::string &uniformName, int uniformValue) const;
	inline void updateUniform(const std::string &uniformName, float uniformValue) const;
	inline void updateUniform(const std::string &uniformName, float x, float y) const;
	inline void updateUniform(const std::string &uniformName, float x, float y, float z) const;
	inline void updateUniform(const std::string &uniformName, float x, float y, float z, float w);
	inline void updateUniform(const std::string &uniformName, const mat4& mat) const;

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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	GeometryPassBlinnPhongShader() {};
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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	LightPassBlinnPhongShader() {};
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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	GeometryPassPBSShader() {};
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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	LightPassPBSShader() {};
};

class BackgroundFPassPBSShader : public GLShader
{
public:
	~BackgroundFPassPBSShader() {};

	static BackgroundFPassPBSShader& getInstance()
	{
		static BackgroundFPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL3DTexture>& textureMap);

private:
	BackgroundFPassPBSShader() {};
};

class BackgroundDPassPBSShader : public GLShader
{
public:
	~BackgroundDPassPBSShader() {};

	static BackgroundDPassPBSShader& getInstance()
	{
		static BackgroundDPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL3DTexture>& textureMap);

private:
	BackgroundDPassPBSShader() {};
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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	FinalPassShader() {};
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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	DebuggerShader() {};
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
};

class GLRenderingManager : public IManager
{
public:
	~GLRenderingManager() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	static GLRenderingManager& getInstance()
	{
		static GLRenderingManager instance;
		return instance;
	}

	meshID addMesh();
	textureID add2DTexture();
	textureID add3DTexture();
	IMesh* getMesh(meshID meshID);
	I2DTexture* get2DTexture(textureID textureID);
	I3DTexture* get3DTexture(textureID textureID);

	void forwardRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap);
	void deferRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);

	void setScreenResolution(vec2 screenResolution);

	void changeDrawPolygonMode();
	void changeDrawTextureMode();

private:
	GLRenderingManager() {};

	vec2 m_screenResolution = vec2();

	std::unordered_map<meshID, GLMesh> m_meshMap;
	std::unordered_map<textureID, GL2DTexture> m_2DTextureMap;
	std::unordered_map<textureID, GL3DTexture> m_3DTextureMap;

	GLuint m_geometryPassFBO;
	GLuint m_geometryPassRT0Texture;
	GLuint m_geometryPassRT1Texture;
	GLuint m_geometryPassRT2Texture;
	GLuint m_geometryPassRT3Texture;
	GLuint m_geometryPassRBO;
	GLShader* m_geometryPassShader;

	GLuint m_lightPassFBO;
	GLuint m_lightPassRT0Texture;
	GLuint m_lightPassRBO;
	GLuint m_lightPassVAO;
	GLuint m_lightPassVBO;
	std::vector<float> m_lightPassVertices;
	GLShader* m_lightPassShader;

	GLuint m_backgroundFPassFBO;
	GLuint m_backgroundFPassRT0Texture;
	GLuint m_backgroundFPassRBO;
	GLuint m_backgroundFPassVAO;
	GLuint m_backgroundFPassVBO;
	std::vector<float> m_backgroundFPassVertices;

	GLuint m_backgroundDPassFBO;
	GLuint m_backgroundDPassRT0Texture;
	GLuint m_backgroundDPassRBO;
	GLuint m_backgroundDPassVAO;
	GLuint m_backgroundDPassVBO;
	std::vector<float> m_backgroundDPassVertices;

	GLShader* m_backgroundFPassShader;
	GLShader* m_backgroundDPassShader;

	GLuint m_finalPassVAO;
	GLuint m_finalPassVBO;
	std::vector<float> m_screenVertices;

	int m_polygonMode = 0;
	int m_textureMode = 0;

	void initializeGeometryPass();
	void renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeLightPass();
	void renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeBackgroundPass();
	void renderBackgroundPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeFinalPass();
	void renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
};