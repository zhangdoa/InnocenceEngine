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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents) override;

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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GLMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

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
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents) override;

private:
	LightPassPBSShader() {};
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
	textureID add2DHDRTexture();
	textureID add3DTexture();
	textureID add3DHDRTexture();
	IMesh* getMesh(meshID meshID);
	I2DTexture* get2DTexture(textureID textureID);
	I2DTexture* get2DHDRTexture(textureID textureID);
	I3DTexture* get3DTexture(textureID textureID);
	I3DTexture* get3DHDRTexture(textureID textureID);

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
	std::unordered_map<textureID, GL2DHDRTexture> m_2DHDRTextureMap;
	std::unordered_map<textureID, GL3DTexture> m_3DTextureMap;
	std::unordered_map<textureID, GL3DHDRTexture> m_3DHDRTextureMap;

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