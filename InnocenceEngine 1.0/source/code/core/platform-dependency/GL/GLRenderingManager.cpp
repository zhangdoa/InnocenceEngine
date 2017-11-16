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

PhongShader::PhongShader()
{
}

PhongShader::~PhongShader()
{
}

void PhongShader::init()
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
	updateUniform("uni_diffuseTexture", 1);
	updateUniform("uni_specularTexture", 2);
	updateUniform("uni_normalTexture", 3);
}

void PhongShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
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

BillboardShader::BillboardShader()
{
}

BillboardShader::~BillboardShader()
{
}

void BillboardShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/billboardVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	addShader(GLShader::FRAGMENT, "GL3.3/billboardFragment.sf");
	bindShader();
	updateUniform("uni_texture", 1);
}

void BillboardShader::draw(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	//bindShader();

	//glm::mat4 m, t, r, p;

	//GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	//GLRenderingManager::getInstance().getCameraRotMatrix(r);
	//GLRenderingManager::getInstance().getCameraPosMatrix(t);
	//m = visibleComponent.getParentActor()->caclTransformationMatrix();
	//// @TODO: multiply with inverse of camera rotation matrix
	//updateUniform("uni_p", p);
	//updateUniform("uni_r", r);
	//updateUniform("uni_t", t);
	//updateUniform("uni_m", m);
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
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	bindShader();

	// TODO: fix "looking outside" problem// almost there
	glm::mat4 r, p;
	GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	GLRenderingManager::getInstance().getCameraRotMatrix(r);

	updateUniform("uni_RP", p * r * -1.0f);
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

	addShader(GLShader::FRAGMENT, "GL3.3/geometryPassFragment.sf");
	bindShader();
	updateUniform("uni_diffuseTexture", 0);
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
		updateUniform("uni_m", visibleComponents[i]->getParentActor()->caclTransformationMatrix());
		visibleComponents[i]->draw();
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

	for (size_t i = 0; i < lightComponents.size(); i++)
	{
		updateUniform("uni_viewPos", cameraComponents[0]->getParentActor()->caclWorldPos());
		updateUniform("uni_Lights.Position", lightComponents[i]->getParentActor()->getTransform()->getPos());
		updateUniform("uni_Lights.Color", lightComponents[i]->getDiffuseColor());
		updateUniform("uni_Lights.Linear", lightComponents[i]->getLinearFactor());
		updateUniform("uni_Lights.Quadratic", lightComponents[i]->getQuadraticFactor());
	}
}
GLRenderingManager::GLRenderingManager()
{
}

GLRenderingManager::~GLRenderingManager()
{
}

void GLRenderingManager::render(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	renderGeometryPass(cameraComponents, lightComponents, visibleComponents);
	renderLightPass(cameraComponents, lightComponents, visibleComponents);
}
//
//void GLRenderingManager::render(std::vector<LightComponent*>& lightComponents, VisibleComponent& visibleComponent)
//{
//	switch (visibleComponent.getVisiblilityType())
//	{
//	case visiblilityType::INVISIBLE: break;
//	case visiblilityType::BILLBOARD:
//		BillboardShader::getInstance().draw(lightComponents, visibleComponent);
//		// update visibleGameEntity's mesh& texture
//		visibleComponent.draw();
//		break;
//	case visiblilityType::STATIC_MESH:
//		for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
//		{
//			m_staticMeshGLShader[i]->draw(lightComponents, visibleComponent);
//		}
//		// update visibleGameEntity's mesh& texture
//		visibleComponent.draw();
//		break;
//	case visiblilityType::SKYBOX:
//		glDepthFunc(GL_LEQUAL);
//		SkyboxShader::getInstance().draw(lightComponents, visibleComponent);
//		// update visibleGameEntity's mesh& texture
//		visibleComponent.draw();
//		glDepthFunc(GL_LESS);
//		break;
//	case visiblilityType::GLASSWARE:
//		for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
//		{
//			m_staticMeshGLShader[i]->draw(lightComponents, visibleComponent);
//		}
//		// update visibleGameEntity's mesh& texture
//		visibleComponent.draw();
//		break;
//	}
//}

void GLRenderingManager::setScreenResolution(glm::vec2 screenResolution)
{
	m_screenResolution = screenResolution;
}

void GLRenderingManager::getCameraPosMatrix(glm::mat4 & t) const
{
	t = cameraPosMatrix;
}

void GLRenderingManager::setCameraPosMatrix(const glm::mat4 & t)
{
	cameraPosMatrix = t;
}


void GLRenderingManager::getCameraRotMatrix(glm::mat4 & v) const
{
	v = cameraRotMatrix;
}

void GLRenderingManager::setCameraRotMatrix(const glm::mat4 & v)
{
	cameraRotMatrix = v;
}

void GLRenderingManager::getCameraProjectionMatrix(glm::mat4 & p) const
{
	p = cameraProjectionMatrix;
}

void GLRenderingManager::setCameraProjectionMatrix(const glm::mat4 & p)
{
	cameraProjectionMatrix = p;
}

void GLRenderingManager::getCameraPos(glm::vec3 & pos) const
{
	pos = cameraPos;
}

void GLRenderingManager::setCameraPos(const glm::vec3 & pos)
{
	cameraPos = pos;
}

void GLRenderingManager::initializeGeometryPass()
{
	// initialize shader
	GeometryPassShader::getInstance().init();
	LightPassShader::getInstance().init();

	glGenFramebuffers(1, &m_gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);

	// position color buffer
	glGenTextures(1, &m_gPosition);
	glBindTexture(GL_TEXTURE_2D, m_gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gPosition, 0);

	// normal color buffer
	glGenTextures(1, &m_gNormal);
	glBindTexture(GL_TEXTURE_2D, m_gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gNormal, 0);

	// albedo color buffer
	glGenTextures(1, &m_gAlbedo);
	glBindTexture(GL_TEXTURE_2D, m_gAlbedo);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_gAlbedo, 0);

	// specular color buffer
	glGenTextures(1, &m_gSpecular);
	glBindTexture(GL_TEXTURE_2D, m_gSpecular);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_screenResolution.x, m_screenResolution.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, m_gSpecular, 0);

	// tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	// create and attach depth buffer (renderbuffer)
	glGenRenderbuffers(1, &m_rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_screenResolution.x, m_screenResolution.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

	// finally check if framebuffer is complete
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		LogManager::getInstance().printLog("Framebuffer not complete!");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	m_screenVertices = {
		-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 0.0f, 1.0f, 0.0f, };

	glGenVertexArrays(1, &m_screenVAO);
	glGenBuffers(1, &m_screenVBO);
	glBindVertexArray(m_screenVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_screenVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(m_screenVertices), &m_screenVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void GLRenderingManager::renderGeometryPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_TEXTURE_2D);

	glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);

	//glFrontFace(GL_CW);
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);

	GeometryPassShader::getInstance().draw(cameraComponents, lightComponents, visibleComponents);
}

void GLRenderingManager::renderLightPass(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_gPosition);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_gNormal);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, m_gAlbedo);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, m_gSpecular);

	LightPassShader::getInstance().draw(cameraComponents, lightComponents, visibleComponents);

	// write to default framebuffer
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gBuffer);
	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); 
	//
	////// blit to default framebuffer
	//glBlitFramebuffer(0, 0, m_screenResolution.x, m_screenResolution.y, 0, 0, m_screenResolution.x, m_screenResolution.y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	//glBindFramebuffer(GL_FRAMEBUFFER, 0);

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
	//BillboardShader::getInstance().init();
	//SkyboxShader::getInstance().init();

	initializeGeometryPass();
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


