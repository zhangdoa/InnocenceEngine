#include "../../../main/stdafx.h"
#include "GLRenderingManager.h"


GLShader::GLShader()
{
}

inline void GLShader::addShader(shaderType shaderType, const std::string & fileLocation) const
{
	attachShader(shaderType, AssetManager::getInstance().loadShader(fileLocation), m_program);
}

inline void GLShader::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName) const
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
	if (glGetAttribLocation(m_program, arrtributeName.c_str()) == 0xFFFFFFFF)
	{
		LogManager::getInstance().printLog("Error: Attribute lost: " + arrtributeName);
	}
}

inline void GLShader::bindShader() const
{
	glUseProgram(m_program);
}

inline void GLShader::initProgram()
{
	m_program = glCreateProgram();
}

inline void GLShader::addUniform(std::string uniform) const
{
	int uniformLocation = glGetUniformLocation(m_program, uniform.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		LogManager::getInstance().printLog("Error: Uniform lost: " + uniform);
	}
}

inline void GLShader::updateUniform(const std::string & uniformName, bool uniformValue) const
{
	glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), (int)uniformValue);
}

inline void GLShader::updateUniform(const std::string & uniformName, int uniformValue) const
{
	glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
}

inline void GLShader::updateUniform(const std::string & uniformName, float uniformValue) const
{
	glUniform1f(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
}

inline void GLShader::updateUniform(const std::string & uniformName, const glm::vec2 & uniformValue) const
{
	glUniform2fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, &uniformValue[0]);
}

inline void GLShader::updateUniform(const std::string & uniformName, float x, float y) const
{
	glUniform2f(glGetUniformLocation(m_program, uniformName.c_str()), x, y);
}

inline void GLShader::updateUniform(const std::string & uniformName, const glm::vec3 & uniformValue) const
{
	glUniform3fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, &uniformValue[0]);
}

inline void GLShader::updateUniform(const std::string & uniformName, float x, float y, float z) const
{
	glUniform3f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z);
}

inline void GLShader::updateUniform(const std::string & uniformName, float x, float y, float z, float w)
{
	glUniform4f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z, w);
}

inline void GLShader::updateUniform(const std::string & uniformName, const glm::mat4 & mat) const
{
	glUniformMatrix4fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, GL_FALSE, &mat[0][0]);
}


GLShader::~GLShader()
{
}

inline void GLShader::attachShader(shaderType shaderType, const std::string& shaderFileContent, int m_program) const
{
	int l_glShaderType = 0;

	switch (shaderType)
	{
	case VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: LogManager::getInstance().printLog("Unknown shader type, cannot add program!");
		break;
	}

	int l_shader = glCreateShader(l_glShaderType);

	if (l_shader == 0) {
		LogManager::getInstance().printLog("Shader creation failed: memory location invaild when adding shader!");
	}

	char const * sourcePointer = shaderFileContent.c_str();
	glShaderSource(l_shader, 1, &sourcePointer, NULL);

	GLint Result = GL_FALSE;
	int l_infoLogLength;

	glGetShaderiv(m_program, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(m_program, GL_INFO_LOG_LENGTH, &l_infoLogLength);

	if (l_infoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(l_infoLogLength + 1);
		glGetShaderInfoLog(m_program, l_infoLogLength, NULL, &ShaderErrorMessage[0]);
		LogManager::getInstance().printLog(&ShaderErrorMessage[0]);
	}

	glAttachShader(m_program, l_shader);

	compileShader();
	GLint success;
	GLchar infoLog[1024];
	glGetShaderiv(l_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(l_shader, 1024, NULL, infoLog);
		std::cout << "Shader compile error: " << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
	}
	detachShader(l_shader);
}

inline void GLShader::compileShader() const
{
	glLinkProgram(m_program);

	glValidateProgram(m_program);
	LogManager::getInstance().printLog("Shader is compiled.");
}

inline void GLShader::detachShader(int shader) const
{
	//glDetachShader(m_program, shader);
	glDeleteShader(shader);
}

forwardBlinnPhongShader::forwardBlinnPhongShader()
{
}

forwardBlinnPhongShader::~forwardBlinnPhongShader()
{
}

void forwardBlinnPhongShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/phongVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	setAttributeLocation(2, "in_Normal");
	setAttributeLocation(3, "in_Tangent");
	setAttributeLocation(4, "in_Bitangent");

	addShader(GLShader::FRAGMENT, "GL3.3/phongFragment.sf");
	bindShader();
	updateUniform("uni_skyboxTexture", 0);
	updateUniform("uni_albedoTexture", 1);
	updateUniform("uni_specularTexture", 2);
	updateUniform("uni_normalTexture", 3);
}

void forwardBlinnPhongShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	//glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	//bindShader();

	//glm::mat4 m, t, r, p;
	//GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	//GLRenderingManager::getInstance().getCameraRotMatrix(r);
	//GLRenderingManager::getInstance().getCameraPosMatrix(t);
	//m = visibleComponent.getParentActor()->caclTransformationMatrix();

	//glm::vec3 cameraPos;
	//GLRenderingManager::getInstance().getCameraPos(cameraPos);

	//updateUniform("uni_p", p);
	//updateUniform("uni_r", r);
	//updateUniform("uni_t", t);
	//updateUniform("uni_m", m);

	//int l_pointLightIndexOffset = 0;
	//for (size_t i = 0; i < lightComponents.size(); i++)
	//{
	//	//@TODO: generalization

	//	updateUniform("uni_viewPos", cameraPos);

	//	if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
	//	{
	//		l_pointLightIndexOffset -= 1;
	//		updateUniform("uni_dirLight.direction", lightComponents[i]->getDirection());
	//		updateUniform("uni_dirLight.ambientColor", lightComponents[i]->getAmbientColor());
	//		updateUniform("uni_dirLight.diffuseColor", lightComponents[i]->getDiffuseColor());
	//		updateUniform("uni_dirLight.specularColor", lightComponents[i]->getSpecularColor());
	//	}
	//	else if (lightComponents[i]->getLightType() == lightType::POINT)
	//	{
	//		std::stringstream ss;
	//		ss << i + l_pointLightIndexOffset;
	//		updateUniform("uni_pointLights[" + ss.str() + "].position", lightComponents[i]->getParentActor()->getTransform()->getPos());
	//		updateUniform("uni_pointLights[" + ss.str() + "].constantFactor", lightComponents[i]->getConstantFactor());
	//		updateUniform("uni_pointLights[" + ss.str() + "].linearFactor", lightComponents[i]->getLinearFactor());
	//		updateUniform("uni_pointLights[" + ss.str() + "].quadraticFactor", lightComponents[i]->getQuadraticFactor());
	//		updateUniform("uni_pointLights[" + ss.str() + "].ambientColor", lightComponents[i]->getAmbientColor());
	//		updateUniform("uni_pointLights[" + ss.str() + "].diffuseColor", lightComponents[i]->getDiffuseColor());
	//		updateUniform("uni_pointLights[" + ss.str() + "].specularColor", lightComponents[i]->getSpecularColor());
	//	}
	//}
	//if (visibleComponent.getVisiblilityType() == visiblilityType::GLASSWARE)
	//{
	//	updateUniform("uni_isGlassware", true);
	//}
	//else
	//{
	//	updateUniform("uni_isGlassware", false);
	//}
}

BillboardPassShader::BillboardPassShader()
{
}

BillboardPassShader::~BillboardPassShader()
{
}

void BillboardPassShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/billboardPassVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	addShader(GLShader::FRAGMENT, "GL3.3/billboardPassFragment.sf");
	bindShader();
	updateUniform("uni_texture", 0);
}

void BillboardPassShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	bindShader();

	glm::mat4 p = cameraComponents[0]->getProjectionMatrix();
	glm::mat4 r = cameraComponents[0]->getRotMatrix();
	glm::mat4 t = cameraComponents[0]->getPosMatrix();
	glm::mat4 m = glm::mat4();

	// @TODO: multiply with inverse of camera rotation matrix
	updateUniform("uni_p", p);
	updateUniform("uni_r", r);
	updateUniform("uni_t", t);

	for (unsigned int i = 0; i < visibleComponents.size(); i++)
	{
		if (visibleComponents[i]->getVisiblilityType() == visiblilityType::BILLBOARD)
		{
			updateUniform("uni_m", visibleComponents[i]->getParentActor()->caclTransformationMatrix());
			visibleComponents[i]->draw();
		}
	}
}

SkyboxShader::SkyboxShader()
{
}

SkyboxShader::~SkyboxShader()
{
}

void SkyboxShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/skyboxVertex.sf");
	setAttributeLocation(0, "in_Position");
	addShader(GLShader::FRAGMENT, "GL3.3/skyboxFragment.sf");
	bindShader();
	updateUniform("uni_skybox", 0);
}

void SkyboxShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glDepthFunc(GL_LEQUAL);

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);

	bindShader();

	// TODO: fix "looking outside" problem// almost there
	glm::mat4 r, p;
	p = cameraComponents[0]->getProjectionMatrix();
	r = cameraComponents[0]->getRotMatrix();

	updateUniform("uni_RP", p * r * -1.0f);
	for (unsigned int i = 0; i < visibleComponents.size(); i++)
	{
		if (visibleComponents[i]->getVisiblilityType() == visiblilityType::SKYBOX)
		{
			visibleComponents[i]->draw();
		}
	}

	glDisable(GL_CULL_FACE);

	glDepthFunc(GL_LESS);
}

GeometryPassShader::GeometryPassShader()
{
}
GeometryPassShader::~GeometryPassShader()
{
}

void GeometryPassShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/geometryPassVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	setAttributeLocation(2, "in_Normal");
	setAttributeLocation(3, "in_Tangent");
	setAttributeLocation(4, "in_Bitangent");

	addShader(GLShader::FRAGMENT, "GL3.3/geometryPassFragment.sf");
	bindShader();
	updateUniform("uni_albedoTexture", 0);
	updateUniform("uni_specularTexture", 1);
	updateUniform("uni_normalTexture", 2);
}

void GeometryPassShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	bindShader();

	glm::mat4 p = cameraComponents[0]->getProjectionMatrix();
	glm::mat4 r = cameraComponents[0]->getRotMatrix();
	glm::mat4 t = cameraComponents[0]->getPosMatrix();
	glm::mat4 m = glm::mat4();

	updateUniform("uni_p", p);
	updateUniform("uni_r", r);
	updateUniform("uni_t", t);

	for (unsigned int i = 0; i < visibleComponents.size(); i++)
	{
		if (visibleComponents[i]->getVisiblilityType() == visiblilityType::STATIC_MESH)
		{
			updateUniform("uni_m", visibleComponents[i]->getParentActor()->caclTransformationMatrix());
			visibleComponents[i]->draw();
		}
	}
}

LightPassShader::LightPassShader()
{
}

LightPassShader::~LightPassShader()
{
}

void LightPassShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/lightPassVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");

	addShader(GLShader::FRAGMENT, "GL3.3/lightPassFragment.sf");
	bindShader();
	updateUniform("m_gPosition", 0);
	updateUniform("m_gNormal", 1);
	updateUniform("m_gAlbedo", 2);
	updateUniform("m_gSpecular", 3);
}

void LightPassShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	bindShader();

	glm::vec3 cameraPos = cameraComponents[0]->getParentActor()->getTransform()->getPos();

	int l_pointLightIndexOffset = 0;
	for (size_t i = 0; i < lightComponents.size(); i++)
	{
		//@TODO: generalization

		updateUniform("uni_viewPos", cameraPos);

		if (lightComponents[i]->getLightType() == lightType::DIRECTIONAL)
		{
			l_pointLightIndexOffset -= 1;
			updateUniform("uni_dirLight.direction", lightComponents[i]->getDirection());
			updateUniform("uni_dirLight.ambientColor", lightComponents[i]->getAmbientColor());
			updateUniform("uni_dirLight.diffuseColor", lightComponents[i]->getDiffuseColor());
			updateUniform("uni_dirLight.specularColor", lightComponents[i]->getSpecularColor());
		}
		else if (lightComponents[i]->getLightType() == lightType::POINT)
		{
			std::stringstream ss;
			ss << i + l_pointLightIndexOffset;
			updateUniform("uni_pointLights[" + ss.str() + "].position", lightComponents[i]->getParentActor()->getTransform()->getPos());
			updateUniform("uni_pointLights[" + ss.str() + "].radius", lightComponents[i]->getRadius());
			updateUniform("uni_pointLights[" + ss.str() + "].constantFactor", lightComponents[i]->getConstantFactor());
			updateUniform("uni_pointLights[" + ss.str() + "].linearFactor", lightComponents[i]->getLinearFactor());
			updateUniform("uni_pointLights[" + ss.str() + "].quadraticFactor", lightComponents[i]->getQuadraticFactor());
			updateUniform("uni_pointLights[" + ss.str() + "].ambientColor", lightComponents[i]->getAmbientColor());
			updateUniform("uni_pointLights[" + ss.str() + "].diffuseColor", lightComponents[i]->getDiffuseColor());
			updateUniform("uni_pointLights[" + ss.str() + "].specularColor", lightComponents[i]->getSpecularColor());
		}
	}
}

FinalPassShader::FinalPassShader()
{
}

FinalPassShader::~FinalPassShader()
{
}

void FinalPassShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/finalPassVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");

	addShader(GLShader::FRAGMENT, "GL3.3/finalPassFragment.sf");
	bindShader();
	updateUniform("finalColor", 0);
}

void FinalPassShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	bindShader();
}


GLRenderingManager::GLRenderingManager()
{
}

GLRenderingManager::~GLRenderingManager()
{
}

void GLRenderingManager::forwardRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	// draw billboard
	BillboardPassShader::getInstance().draw(cameraComponents, lightComponents, visibleComponents);
	// draw skybox
	SkyboxShader::getInstance().draw(cameraComponents, lightComponents, visibleComponents);
}

void GLRenderingManager::deferRender(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	renderGeometryPass(cameraComponents, lightComponents, visibleComponents);
	renderLightPass(cameraComponents, lightComponents, visibleComponents);
	renderFinalPass(cameraComponents, lightComponents, visibleComponents);
}

void GLRenderingManager::setScreenResolution(glm::vec2 screenResolution)
{
	m_screenResolution = screenResolution;
}

void GLRenderingManager::initializeGeometryPass()
{
	// initialize shader
	GeometryPassShader::getInstance().init();

	glGenFramebuffers(1, &m_geometryPassBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_geometryPassBuffer);

	// position color buffer
	glGenTextures(1, &m_geometryPassPosition);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_geometryPassPosition, 0);

	// normal color buffer
	glGenTextures(1, &m_geometryPassNormal);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_geometryPassNormal, 0);

	// albedo color buffer
	glGenTextures(1, &m_geometryPassAlbedo);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_geometryPassAlbedo, 0);

	// specular color buffer
	glGenTextures(1, &m_geometryPassSpecular);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassSpecular);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_geometryPassSpecular, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_geometryPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_geometryPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_screenResolution.x, m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_geometryPassRBO);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Framebuffer not complete!");
	}
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderingManager::renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_geometryPassBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_geometryPassRBO);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);

	//glFrontFace(GL_CCW);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	GeometryPassShader::getInstance().draw(cameraComponents, lightComponents, visibleComponents);
}

void GLRenderingManager::initializeLightPass()
{
	// initialize shader
	LightPassShader::getInstance().init();

	glGenFramebuffers(1, &m_lightPassBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_lightPassBuffer);

	// final color buffer
	glGenTextures(1, &m_lightPassFinal);
	glBindTexture(GL_TEXTURE_2D, m_lightPassFinal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_lightPassFinal, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_lightPassRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, m_lightPassRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_screenResolution.x, m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_lightPassRBO);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Framebuffer not complete!");
	}
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLRenderingManager::renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_lightPassBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, m_lightPassRBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassAlbedo);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_geometryPassSpecular);

	unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, attachments);

	LightPassShader::getInstance().draw(cameraComponents, lightComponents, visibleComponents);

	// draw screen rectangle
	glBindVertexArray(m_screenVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLRenderingManager::initializeBillboardPass()
{
}

void GLRenderingManager::renderBillboardPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
}

void GLRenderingManager::initializeFinalPass()
{
	// initialize shader
	FinalPassShader::getInstance().init();

	// initialize screen quad
	m_screenVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_screenVAO);
	glGenBuffers(1, &m_screenVBO);
	glBindVertexArray(m_screenVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_screenVBO);
	// take care of std::vector's size and pointer of first element!!!
	glBufferData(GL_ARRAY_BUFFER, m_screenVertices.size() * sizeof(float), &m_screenVertices[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLRenderingManager::renderFinalPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	FinalPassShader::getInstance().draw(cameraComponents, lightComponents, visibleComponents);

	// write to final pass framebuffer
	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_lightPassBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	//// blit to final pass framebuffer
	glBlitFramebuffer(0, 0, m_screenResolution.x, m_screenResolution.y, 0, 0, m_screenResolution.x, m_screenResolution.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// draw screen rectangle
	glBindVertexArray(m_screenVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}

void GLRenderingManager::changeDrawPolygonMode()
{
	if (m_polygonMode == 3)
	{
		m_polygonMode = 0;
	}
	switch (m_polygonMode)
	{
	case 0: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
	case 1:	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
	case 2: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
	}
	m_polygonMode += 1;
}
void GLRenderingManager::toggleDepthBufferVisualizer()
{
	if (m_drawDepthBuffer)
	{
		m_drawDepthBuffer = false;
	}
	else
	{
		m_drawDepthBuffer = true;
	}
}
bool GLRenderingManager::canDrawDepthBuffer() const
{
	return m_drawDepthBuffer;
}
void GLRenderingManager::initialize()
{
	//PhongShader::getInstance().init();
	//BillboardPassShader::getInstance().init();
	//SkyboxShader::getInstance().init();
	glEnable(GL_TEXTURE_2D);
	initializeGeometryPass();
	initializeLightPass();
	initializeFinalPass();
	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("GLRenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
}

void GLRenderingManager::shutdown()
{
	this->setStatus(objectStatus::SHUTDOWN);
	LogManager::getInstance().printLog("GLRenderingManager has been shutdown.");
}
