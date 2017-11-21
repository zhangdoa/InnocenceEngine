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
	virtual void draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents) = 0;

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

class forwardBlinnPhongShader : public GLShader
{
public:
	~forwardBlinnPhongShader();

	static forwardBlinnPhongShader& getInstance()
	{
		static forwardBlinnPhongShader instance;
		return instance;
	}

	void init() override;
	void draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents) override;

private:
	forwardBlinnPhongShader();
};

class BillboardShader : public GLShader
{
public:
	~BillboardShader();

	static BillboardShader& getInstance()
	{
		static BillboardShader instance;
		return instance;
	}

	void init() override;
	void draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents) override;

private:
	BillboardShader();
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
	void draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents) override;

private:
	SkyboxShader();
};

class GeometryPassShader : public GLShader
{
public:
	~GeometryPassShader();

	static GeometryPassShader& getInstance()
	{
		static GeometryPassShader instance;
		return instance;
	}

	void init() override;
	void draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents) override;

private:
	GeometryPassShader();
};

class LightPassShader : public GLShader
{
public:
	~LightPassShader();

	static LightPassShader& getInstance()
	{
		static LightPassShader instance;
		return instance;
	}

	void init() override;
	void draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents) override;

private:
	LightPassShader();
};

class GLRenderingManager : public IEventManager
{
public:
	~GLRenderingManager();

	static GLRenderingManager& getInstance()
	{
		static GLRenderingManager instance;
		return instance;
	}

	void forwardRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void deferRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);

	void setScreenResolution(glm::vec2 screenResolution);

	void changeDrawPolygonMode();
	void toggleDepthBufferVisualizer();
	bool canDrawDepthBuffer() const;

private:
	GLRenderingManager();

	glm::vec2 m_screenResolution = glm::vec2();

	GLuint m_gBuffer;
	GLuint m_gPosition;
	GLuint m_gNormal;
	GLuint m_gAlbedo;
	GLuint m_gSpecular;
	GLuint m_rbo;

	GLuint m_screenVAO;
	GLuint m_screenVBO;
	std::vector<float> m_screenVertices;

	int m_polygonMode = 0;
	bool m_drawDepthBuffer = false;

	void initialize() override;
	void update() override;
	void shutdown() override;

	void initializeGeometryPass();
	void renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
	void initializeLightPass();
	void renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents);
};