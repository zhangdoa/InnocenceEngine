#include "GLShaderProgram.h"

GLShaderProgram::GLShaderProgram()
{
}

GLShaderProgram::~GLShaderProgram()
{
}

void GLShaderProgram::initialize()
{
	m_program = glCreateProgram();

	for (auto i : m_baseShaders)
	{
		i->initialize();
		attachShader(i);
	}
}

void GLShaderProgram::shutdown()
{
	for (auto i : m_baseShaders)
	{
		if (i->getStatus() == objectStatus::ALIVE)
		{
			i->shutdown();
			glDetachShader(m_program, i->getShaderID());
		}
	}
}

void GLShaderProgram::attachShader(BaseShader* GLShader) const
{
	GLint Result = GL_FALSE;
	int l_infoLogLength = 0;

	glGetShaderiv(m_program, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(m_program, GL_INFO_LOG_LENGTH, &l_infoLogLength);

	if (l_infoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(l_infoLogLength + 1);
		glGetShaderInfoLog(m_program, l_infoLogLength, NULL, &ShaderErrorMessage[0]);
		g_pLogSystem->printLog(&ShaderErrorMessage[0]);
	}

	const GLint l_shaderID = GLShader->getShaderID();

	glAttachShader(m_program, l_shaderID);
	glLinkProgram(m_program);
	glValidateProgram(m_program);

	g_pLogSystem->printLog("innoShader: " + std::get<1>(GLShader->getShaderData()) + " Shader is compiled.");

	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(l_shaderID, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(l_shaderID, 1024, NULL, infoLog);
		g_pLogSystem->printLog("innoShader: " + std::get<1>(GLShader->getShaderData()) + " compile error: " + std::string(infoLog) + "\n -- --------------------------------------------------- -- ");
	}
}

inline GLint GLShaderProgram::getUniformLocation(const std::string & uniformName) const
{
	int uniformLocation = glGetUniformLocation(m_program, uniformName.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		g_pLogSystem->printLog("innoShader: Error: Uniform lost: " + uniformName);
		return -1;
	}
	return uniformLocation;
}

inline void GLShaderProgram::activateUniform(const GLint uniformLocation, bool uniformValue) const
{
	glUniform1i(uniformLocation, (int)uniformValue);
}

inline void GLShaderProgram::activateUniform(const GLint uniformLocation, int uniformValue) const
{
	glUniform1i(uniformLocation, uniformValue);
}

inline void GLShaderProgram::activateUniform(const GLint uniformLocation, double uniformValue) const
{
	glUniform1f(uniformLocation, (GLfloat)uniformValue);
}

inline void GLShaderProgram::activateUniform(const GLint uniformLocation, double x, double y) const
{
	glUniform2f(uniformLocation, (GLfloat)x, (GLfloat)y);
}

inline void GLShaderProgram::activateUniform(const GLint uniformLocation, double x, double y, double z) const
{
	glUniform3f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z);
}

inline void GLShaderProgram::activateUniform(const GLint uniformLocation, double x, double y, double z, double w) const
{
	glUniform4f(uniformLocation, (GLfloat)x, (GLfloat)y, (GLfloat)z, (GLfloat)w);
}

inline void GLShaderProgram::activateUniform(const GLint uniformLocation, const mat4 & mat) const
{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_FALSE, &mat.m[0][0]);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	glUniformMatrix4fv(uniformLocation, 1, GL_TRUE, &mat.m[0][0]);
#endif
}

void EnvironmentCapturePassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_equirectangularMap = getUniformLocation("uni_equirectangularMap");
	activateUniform(m_uni_equirectangularMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void EnvironmentCapturePassPBSShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(m_program);
}

void EnvironmentConvolutionPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	activateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void EnvironmentConvolutionPassPBSShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(m_program);
}

void EnvironmentPreFilterPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_capturedCubeMap = getUniformLocation("uni_capturedCubeMap");
	activateUniform(m_uni_capturedCubeMap, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");

	m_uni_roughness = getUniformLocation("uni_roughness");
}

void EnvironmentPreFilterPassPBSShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(m_program);
}

void EnvironmentBRDFLUTPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);
}

void EnvironmentBRDFLUTPassPBSShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(m_program);
}

void ShadowForwardPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_v = getUniformLocation("uni_v");
	m_uni_m = getUniformLocation("uni_m");
}

void ShadowForwardPassShaderProgram::activate()
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_FRONT);
	glUseProgram(m_program);
}

void ShadowDeferPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_shadowForwardPassRT0 = getUniformLocation("uni_shadowForwardPassRT0");
	activateUniform(m_uni_shadowForwardPassRT0, 0);
}

void ShadowDeferPassShaderProgram::activate()
{
	glUseProgram(m_program);
}

void GeometryPassBlinnPhongShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_normalTexture = getUniformLocation("uni_normalTexture");
	activateUniform(m_uni_normalTexture, 0);
	m_uni_diffuseTexture = getUniformLocation("uni_diffuseTexture");
	activateUniform(m_uni_diffuseTexture, 1);
	m_uni_specularTexture = getUniformLocation("uni_specularTexture");
	activateUniform(m_uni_specularTexture, 2);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void GeometryPassBlinnPhongShaderProgram::activate()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glUseProgram(m_program);
}

void LightPassBlinnPhongShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_RT0 = getUniformLocation("uni_RT0");
	activateUniform(m_uni_RT0, 0);
	m_uni_RT1 = getUniformLocation("uni_RT1");
	activateUniform(m_uni_RT1, 1);
	m_uni_RT2 = getUniformLocation("uni_RT2");
	activateUniform(m_uni_RT2, 2);
	m_uni_RT3 = getUniformLocation("uni_RT3");
	activateUniform(m_uni_RT3, 3);

	m_uni_viewPos = getUniformLocation("uni_viewPos");

	m_uni_dirLight_direction = getUniformLocation("uni_dirLight.direction");
	m_uni_dirLight_color = getUniformLocation("uni_dirLight.color");
}

void LightPassBlinnPhongShaderProgram::activate()
{
	glUseProgram(m_program);
}

void GeometryPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_normalTexture = getUniformLocation("uni_normalTexture");
	activateUniform(m_uni_normalTexture, 0);
	m_uni_albedoTexture = getUniformLocation("uni_albedoTexture");
	activateUniform(m_uni_albedoTexture, 1);
	m_uni_metallicTexture = getUniformLocation("uni_metallicTexture");
	activateUniform(m_uni_metallicTexture, 2);
	m_uni_roughnessTexture = getUniformLocation("uni_roughnessTexture");
	activateUniform(m_uni_roughnessTexture, 3);
	m_uni_aoTexture = getUniformLocation("uni_aoTexture");
	activateUniform(m_uni_aoTexture, 4);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
	m_uni_p_light = getUniformLocation("uni_p_light");
	m_uni_v_light = getUniformLocation("uni_v_light");

	m_uni_useTexture = getUniformLocation("uni_useTexture");
	m_uni_albedo = getUniformLocation("uni_albedo");
	m_uni_MRA = getUniformLocation("uni_MRA");
}

void GeometryPassPBSShaderProgram::activate()
{
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_POINT + (int)in_shaderDrawPair.first);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glStencilMask(0xFF);
	glClear(GL_STENCIL_BUFFER_BIT);

	glUseProgram(m_program);
}

void LightPassPBSShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_geometryPassRT0 = getUniformLocation("uni_geometryPassRT0");
	activateUniform(m_uni_geometryPassRT0, 0);
	m_uni_geometryPassRT1 = getUniformLocation("uni_geometryPassRT1");
	activateUniform(m_uni_geometryPassRT1, 1);
	m_uni_geometryPassRT2 = getUniformLocation("uni_geometryPassRT2");
	activateUniform(m_uni_geometryPassRT2, 2);
	m_uni_geometryPassRT3 = getUniformLocation("uni_geometryPassRT3");
	activateUniform(m_uni_geometryPassRT3, 3);
	m_uni_shadowMap = getUniformLocation("uni_shadowMap");
	activateUniform(m_uni_shadowMap, 4);
	m_uni_irradianceMap = getUniformLocation("uni_irradianceMap");
	activateUniform(m_uni_irradianceMap, 5);
	m_uni_preFiltedMap = getUniformLocation("uni_preFiltedMap");
	activateUniform(m_uni_preFiltedMap, 6);
	m_uni_brdfLUT = getUniformLocation("uni_brdfLUT");
	activateUniform(m_uni_brdfLUT, 7);

	m_uni_textureMode = getUniformLocation("uni_textureMode");
	m_uni_shadingMode = getUniformLocation("uni_shadingMode");

	m_uni_viewPos = getUniformLocation("uni_viewPos");

	m_uni_dirLight_position = getUniformLocation("uni_dirLight.position");
	m_uni_dirLight_direction = getUniformLocation("uni_dirLight.direction");
	m_uni_dirLight_color = getUniformLocation("uni_dirLight.color");
}

void LightPassPBSShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_EQUAL, 0x01, 0xFF);
	glStencilMask(0x00);

	glUseProgram(m_program);
}

void SkyPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_skybox = getUniformLocation("uni_skybox");
	activateUniform(m_uni_skybox, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
}

void SkyPassShaderProgram::activate()
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_POINT + (int)in_shaderDrawPair.first);

	glUseProgram(m_program);
}

void BloomExtractPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_lightPassRT0 = getUniformLocation("uni_lightPassRT0");
	activateUniform(m_uni_lightPassRT0, 0);
}

void BloomExtractPassShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glUseProgram(m_program);
}

void BloomBlurPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_bloomExtractPassRT0 = getUniformLocation("uni_bloomExtractPassRT0");
	activateUniform(m_uni_bloomExtractPassRT0, 0);
	m_uni_horizontal = getUniformLocation("uni_horizontal");
}

void BloomBlurPassShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_POINT + (int)in_shaderDrawPair.first);
	glUseProgram(m_program);
}

void BillboardPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_texture = getUniformLocation("uni_texture");
	activateUniform(m_uni_texture, 0);
	m_uni_pos = getUniformLocation("uni_pos");
	m_uni_albedo = getUniformLocation("uni_albedo");
	m_uni_size = getUniformLocation("uni_size");
	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
}

void BillboardPassShaderProgram::activate()
{
	glEnable(GL_DEPTH_TEST);
	glUseProgram(m_program);
}

void DebuggerShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_normalTexture = getUniformLocation("uni_normalTexture");
	activateUniform(m_uni_normalTexture, 0);

	m_uni_p = getUniformLocation("uni_p");
	m_uni_r = getUniformLocation("uni_r");
	m_uni_t = getUniformLocation("uni_t");
	m_uni_m = getUniformLocation("uni_m");
}

void DebuggerShaderProgram::activate()
{
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(m_program);
}

void FinalPassShaderProgram::initialize()
{
	GLShaderProgram::initialize();
	glUseProgram(m_program);

	m_uni_lightPassRT0 = getUniformLocation("uni_lightPassRT0");
	activateUniform(m_uni_lightPassRT0, 0);
	m_uni_skyPassRT0 = getUniformLocation("uni_skyPassRT0");
	activateUniform(m_uni_skyPassRT0, 1);
	m_uni_bloomPassRT0 = getUniformLocation("uni_bloomPassRT0");
	activateUniform(m_uni_bloomPassRT0, 2);
	m_uni_billboardPassRT0 = getUniformLocation("uni_billboardPassRT0");
	activateUniform(m_uni_billboardPassRT0, 3);
	m_uni_debuggerPassRT0 = getUniformLocation("uni_debuggerPassRT0");
	activateUniform(m_uni_debuggerPassRT0, 4);
}

void FinalPassShaderProgram::activate()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDisable(GL_STENCIL_TEST);

	glUseProgram(m_program);
}
