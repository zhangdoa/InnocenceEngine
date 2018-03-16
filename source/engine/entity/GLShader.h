#pragma once

#include "entity/ComponentHeaders.h"
#include "entity/GLGraphicPrimitive.h"

#include "interface/IAssetSystem.h"
extern IAssetSystem* g_pAssetSystem;

enum class shaderType { VERTEX, GEOMETRY, FRAGMENT };

class GLShader : public IObject
{
public:
	GLShader();
	~GLShader();

	void setup() override;
	void setup(shaderType shaderType, const std::string& shaderFilePath, const std::vector<std::string>& attributions);
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;


	const GLint& getShaderID() const;
	const std::string& getShaderFilePath() const;
	const std::vector<std::string>& getAttributions() const;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	shaderType m_shaderType;
	std::string m_shaderFilePath;
	std::vector<std::string> m_attributions;
	std::string m_shaderCode;

	GLint m_shaderID;

	void parseAttribution();
};

// @TODO: ugly as ugly itself
typedef std::vector<std::tuple<shaderType, std::string, std::vector<std::string>>> shaderTuple;

class GLShaderProgram
{
public:
	virtual ~GLShaderProgram();

	void setup();
	void setup(shaderTuple GLShaders);

	virtual void initialize();
	void shutdown();

	virtual void update() {};

	virtual void update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap) {};
	virtual void update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture& threeDTexture) {};
	virtual void update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) {};

	virtual void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, int textureMode, int shadingMode) {};
	virtual void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) {};

	virtual void update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DHDRTexture>& twoDTextureMap, GL3DHDRTexture& threeDTexture) {};
	virtual void update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture& threeDCapturedTexture, GL3DHDRTexture& threeDConvolutedTexture) {};


protected:
	GLShaderProgram();

	void attachShader(const GLShader* GLShader) const;
	void setAttributeLocation(int arrtributeLocation, const std::string& arrtributeName) const;

	inline void useProgram() const;

	inline void addUniform(std::string uniform) const;
	inline GLint getUniformLocation(const std::string &uniformName) const;

	inline void updateUniform(const GLint uniformLocation, bool uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, int uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z, double w);
	inline void updateUniform(const GLint uniformLocation, const mat4& mat) const;

	GLShader m_vertexShader;
	GLShader m_geometryShader;
	GLShader m_fragmentShader;

	unsigned int m_program;
};

class GeometryPassBlinnPhongShaderProgram : public GLShaderProgram
{
public:
	GeometryPassBlinnPhongShaderProgram() {};
	~GeometryPassBlinnPhongShaderProgram() {};

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	GLint m_uni_normalTexture;
	GLint m_uni_diffuseTexture;
	GLint m_uni_specularTexture;
	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;
};

class LightPassBlinnPhongShaderProgram : public GLShaderProgram
{
public:
	LightPassBlinnPhongShaderProgram() {};
	~LightPassBlinnPhongShaderProgram() {};

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, int textureMode, int shadingMode) override;

private:
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

class GeometryPassPBSShaderProgram : public GLShaderProgram
{
public:
	GeometryPassPBSShaderProgram() {};
	~GeometryPassPBSShaderProgram() {};

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
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

class LightPassPBSShaderProgram : public GLShaderProgram
{
public:
	LightPassPBSShaderProgram() {};
	~LightPassPBSShaderProgram() {};

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, int textureMode, int shadingMode) override;

private:
	GLint m_uni_geometryPassRT0;
	GLint m_uni_geometryPassRT1;
	GLint m_uni_geometryPassRT2;
	GLint m_uni_geometryPassRT3;
	GLint m_uni_irradianceMap;
	GLint m_uni_preFiltedMap;
	GLint m_uni_brdfLUT;
	GLint m_uni_textureMode;
	GLint m_uni_shadingMode;

	GLint m_uni_viewPos;
	GLint m_uni_dirLight_direction;
	GLint m_uni_dirLight_color;

	std::vector<GLint> m_uni_pointLights_position;
	std::vector<GLint> m_uni_pointLights_radius;
	std::vector<GLint> m_uni_pointLights_color;

	bool isPointLightUniformAdded = false;
};

class EnvironmentCapturePassPBSShaderProgram : public GLShaderProgram
{
public:
	EnvironmentCapturePassPBSShaderProgram() {};
	~EnvironmentCapturePassPBSShaderProgram() {};

	void initialize() override;
	void update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DHDRTexture>& twoDTextureMap, GL3DHDRTexture& threeDTexture);

private:
	GLint m_uni_equirectangularMap;

	GLint m_uni_p;
	GLint m_uni_r;
};

class EnvironmentConvolutionPassPBSShaderProgram : public GLShaderProgram
{
public:
	EnvironmentConvolutionPassPBSShaderProgram() {};
	~EnvironmentConvolutionPassPBSShaderProgram() {};

	void initialize() override;
	void update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture& threeDCapturedTexture, GL3DHDRTexture& threeDConvolutedTexture);

private:
	GLint m_uni_capturedCubeMap;

	GLint m_uni_p;
	GLint m_uni_r;
};

class EnvironmentPreFilterPassPBSShaderProgram : public GLShaderProgram
{
public:
	EnvironmentPreFilterPassPBSShaderProgram() {};
	~EnvironmentPreFilterPassPBSShaderProgram() {};

	void initialize() override;
	void update(std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture& threeDCapturedTexture, GL3DHDRTexture& threeDPreFiltedTexture);

private:
	GLint m_uni_capturedCubeMap;

	GLint m_uni_p;
	GLint m_uni_r;

	GLint m_uni_roughness;
};

class EnvironmentBRDFLUTPassPBSShaderProgram : public GLShaderProgram
{
public:
	EnvironmentBRDFLUTPassPBSShaderProgram() {};
	~EnvironmentBRDFLUTPassPBSShaderProgram() {};

	static EnvironmentBRDFLUTPassPBSShaderProgram& getInstance()
	{
		static EnvironmentBRDFLUTPassPBSShaderProgram instance;
		return instance;
	}

	void initialize() override;
	void update();
};

class SkyForwardPassPBSShaderProgram : public GLShaderProgram
{
public:
	SkyForwardPassPBSShaderProgram() {};
	~SkyForwardPassPBSShaderProgram() {};

	static SkyForwardPassPBSShaderProgram& getInstance()
	{
		static SkyForwardPassPBSShaderProgram instance;
		return instance;
	}

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, GL3DHDRTexture& threeDTexture);

private:
	GLint m_uni_skybox;

	GLint m_uni_p;
	GLint m_uni_r;
};

class DebuggerShaderProgram : public GLShaderProgram
{
public:
	DebuggerShaderProgram() {};
	~DebuggerShaderProgram() {};

	static DebuggerShaderProgram& getInstance()
	{
		static DebuggerShaderProgram instance;
		return instance;
	}

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap) override;

private:
	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;
};

class SkyDeferPassPBSShaderProgram : public GLShaderProgram
{
public:
	SkyDeferPassPBSShaderProgram() {};
	~SkyDeferPassPBSShaderProgram() {};

	static SkyDeferPassPBSShaderProgram& getInstance()
	{
		static SkyDeferPassPBSShaderProgram instance;
		return instance;
	}

	void initialize() override;
	void update();

private:
	GLint m_uni_lightPassRT0;
	GLint m_uni_skyForwardPassRT0;
	GLint m_uni_debuggerPassRT0;
};

class FinalPassShaderProgram : public GLShaderProgram
{
public:
	FinalPassShaderProgram() {};
	~FinalPassShaderProgram() {};

	static FinalPassShaderProgram& getInstance()
	{
		static FinalPassShaderProgram instance;
		return instance;
	}

	void initialize() override;
	void update() override;

private:
	GLint m_uni_skyDeferPassRT0;
};

class BillboardPassShaderProgram : public GLShaderProgram
{
public:
	BillboardPassShaderProgram() {};
	~BillboardPassShaderProgram() {};

	static BillboardPassShaderProgram& getInstance()
	{
		static BillboardPassShaderProgram instance;
		return instance;
	}

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, GL3DMesh>& meshMap, std::unordered_map<EntityID, GL2DTexture>& textureMap) override;

private:
	GLint m_uni_texture;
	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;
};