#pragma once
#include <sstream>
#include "BaseShaderProgram.h"
#include "GLShader.h"

class GLShaderProgram : public BaseShaderProgram
{
public:
	GLShaderProgram();
	virtual ~GLShaderProgram();

	void initialize() override;
	void shutdown() override;

	void attachShader(BaseShader* GLShader) const;
	inline void useProgram() const;
	inline GLint getUniformLocation(const std::string &uniformName) const;
	inline void updateUniform(const GLint uniformLocation, bool uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, int uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double uniformValue) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z) const;
	inline void updateUniform(const GLint uniformLocation, double x, double y, double z, double w) const;
	inline void updateUniform(const GLint uniformLocation, const mat4& mat) const;

protected:
	GLint m_program = 0;
};

class EnvironmentCapturePassPBSShaderProgram : public GLShaderProgram
{
public:
	EnvironmentCapturePassPBSShaderProgram() {};
	~EnvironmentCapturePassPBSShaderProgram() {};

	void initialize() override;
	void update() override;

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
	void update() override;

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
	void update() override;

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


	void initialize() override;
	void update() override;
};

class ShadowForwardPassShaderProgram : public GLShaderProgram
{
public:
	ShadowForwardPassShaderProgram() {};
	~ShadowForwardPassShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_p;
	GLint m_uni_v;
	GLint m_uni_m;
};

class ShadowDeferPassShaderProgram : public GLShaderProgram
{
public:
	ShadowDeferPassShaderProgram() {};
	~ShadowDeferPassShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_shadowForwardPassRT0;
};

class GeometryPassBlinnPhongShaderProgram : public GLShaderProgram
{
public:
	GeometryPassBlinnPhongShaderProgram() {};
	~GeometryPassBlinnPhongShaderProgram() {};

	void initialize() override;
	void update() override;

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
	void update() override;

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
	void update() override;

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
	GLint m_uni_p_light;
	GLint m_uni_v_light;

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
	void update() override;

private:
	GLint m_uni_geometryPassRT0;
	GLint m_uni_geometryPassRT1;
	GLint m_uni_geometryPassRT2;
	GLint m_uni_geometryPassRT3;
	GLint m_uni_shadowMap;
	GLint m_uni_irradianceMap;
	GLint m_uni_preFiltedMap;
	GLint m_uni_brdfLUT;
	GLint m_uni_textureMode;
	GLint m_uni_shadingMode;

	GLint m_uni_viewPos;
	GLint m_uni_dirLight_position;
	GLint m_uni_dirLight_direction;
	GLint m_uni_dirLight_color;

	std::vector<GLint> m_uni_pointLights_position;
	std::vector<GLint> m_uni_pointLights_radius;
	std::vector<GLint> m_uni_pointLights_color;

	bool isPointLightUniformAdded = false;
};

class SkyPassShaderProgram : public GLShaderProgram
{
public:
	SkyPassShaderProgram() {};
	~SkyPassShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_skybox;

	GLint m_uni_p;
	GLint m_uni_r;
};

class BloomExtractPassShaderProgram : public GLShaderProgram
{
public:
	BloomExtractPassShaderProgram() {};
	~BloomExtractPassShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_lightPassRT0;
};

class BloomBlurPassShaderProgram : public GLShaderProgram
{
public:
	BloomBlurPassShaderProgram() {};
	~BloomBlurPassShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_bloomExtractPassRT0;
	GLint m_uni_horizontal;
	bool m_isHorizontal = true;
};

class BillboardPassShaderProgram : public GLShaderProgram
{
public:
	BillboardPassShaderProgram() {};
	~BillboardPassShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_texture;
	GLint m_uni_pos;
	GLint m_uni_albedo;
	GLint m_uni_size;
	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
};

class DebuggerShaderProgram : public GLShaderProgram
{
public:
	DebuggerShaderProgram() {};
	~DebuggerShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_normalTexture;

	GLint m_uni_p;
	GLint m_uni_r;
	GLint m_uni_t;
	GLint m_uni_m;
};

class FinalPassShaderProgram : public GLShaderProgram
{
public:
	FinalPassShaderProgram() {};
	~FinalPassShaderProgram() {};

	void initialize() override;
	void update() override;

private:
	GLint m_uni_lightPassRT0;
	GLint m_uni_skyPassRT0;
	GLint m_uni_bloomPassRT0;
	GLint m_uni_billboardPassRT0;
	GLint m_uni_debuggerPassRT0;
};
