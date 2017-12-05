#pragma once
#include "../../interface/IEventManager.h"
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
	virtual void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) = 0;

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
	inline void updateUniform(const std::string &uniformName, const glm::vec2 &uniformValue) const;
	inline void updateUniform(const std::string &uniformName, float x, float y) const;
	inline void updateUniform(const std::string &uniformName, const glm::vec3& uniformValue) const;
	inline void updateUniform(const std::string &uniformName, float x, float y, float z) const;
	inline void updateUniform(const std::string &uniformName, float x, float y, float z, float w);
	inline void updateUniform(const std::string &uniformName, const glm::mat4& mat) const;

private:
	inline void attachShader(shaderType shaderType, const std::string& fileContent, int m_program) const;
	inline void compileShader() const;
	inline void detachShader(int shader) const;

	unsigned int m_program;
};

class SkyboxShader : public GLShader
{
public:
	~SkyboxShader();

	static SkyboxShader& getInstance()
	{
		static SkyboxShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) override;

private:
	SkyboxShader();
};

class GeometryPassBlinnPhongShader : public GLShader
{
public:
	~GeometryPassBlinnPhongShader();

	static GeometryPassBlinnPhongShader& getInstance()
	{
		static GeometryPassBlinnPhongShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) override;

private:
	GeometryPassBlinnPhongShader();
};

class LightPassBlinnPhongShader : public GLShader
{
public:
	~LightPassBlinnPhongShader();

	static LightPassBlinnPhongShader& getInstance()
	{
		static LightPassBlinnPhongShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) override;

private:
	LightPassBlinnPhongShader();
};

class GeometryPassPBSShader : public GLShader
{
public:
	~GeometryPassPBSShader();

	static GeometryPassPBSShader& getInstance()
	{
		static GeometryPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) override;

private:
	GeometryPassPBSShader();
};

class LightPassPBSShader : public GLShader
{
public:
	~LightPassPBSShader();

	static LightPassPBSShader& getInstance()
	{
		static LightPassPBSShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) override;

private:
	LightPassPBSShader();
};

class FinalPassShader : public GLShader
{
public:
	~FinalPassShader();

	static FinalPassShader& getInstance()
	{
		static FinalPassShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) override;

private:
	FinalPassShader();
};

class BillboardPassShader : public GLShader
{
public:
	~BillboardPassShader();

	static BillboardPassShader& getInstance()
	{
		static BillboardPassShader instance;
		return instance;
	}

	void init() override;
	void shaderDraw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas) override;

private:
	BillboardPassShader();
};

class GLRenderingManager : public IEventManager
{
public:
	~GLRenderingManager();

	void initialize() override;
	void update() override;
	void shutdown() override;

	static GLRenderingManager& getInstance()
	{
		static GLRenderingManager instance;
		return instance;
	}

	void forwardRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData>& meshDatas, std::unordered_map<unsigned long int, TextureData>& textureDatas);
	void deferRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData>& meshDatas, std::unordered_map<unsigned long int, TextureData>& textureDatas);

	void setScreenResolution(glm::vec2 screenResolution);

	void changeDrawPolygonMode();
	void toggleDepthBufferVisualizer();
	bool canDrawDepthBuffer() const;

private:
	GLRenderingManager();

	glm::vec2 m_screenResolution = glm::vec2();

	GLuint m_geometryPassFBO;
	GLuint m_geometryPassPositionTexture;
	GLuint m_geometryPassNormalTexture;
	GLuint m_geometryPassAlbedoTexture;
	GLuint m_geometryPassSpecularTexture;
	GLuint m_geometryPassRBO;

	GLuint m_lightPassFBO;
	GLuint m_lightPassFinalTexture;
	GLuint m_lightPassRBO;
	GLuint m_lightPassVAO;
	GLuint m_lightPassVBO;
	std::vector<float> m_lightPassVertices;

	GLuint m_screenVAO;
	GLuint m_screenVBO;
	std::vector<float> m_screenVertices;

	int m_polygonMode = 0;
	bool m_drawDepthBuffer = false;

	void initializeGeometryPass();
	void renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas);
	void initializeLightPass();
	void renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas);
	void initializeBillboardPass();
	void renderBillboardPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas);
	void initializeFinalPass();
	void renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<unsigned long int, MeshData> meshDatas, std::unordered_map<unsigned long int, TextureData> textureDatas);
};