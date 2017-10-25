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
	//m_uniforms.emplace(std::pair<std::string, int>(uniform.c_str(), uniformLocation));
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
	LogManager::getInstance().printLog("Shader compiled.");
}

inline void GLShader::detachShader(int shader) const
{
	//glDetachShader(m_program, shader);
	glDeleteShader(shader);
}

BasicGLShader::BasicGLShader()
{
}

BasicGLShader::~BasicGLShader()
{
}

void BasicGLShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/basicVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	//setAttributeLocation(2, "in_Normal");
	addShader(GLShader::FRAGMENT, "GL3.3/basicFragment.sf");
	updateUniform("uni_Texture", 0);
}

void BasicGLShader::draw(LightComponent& lightComponent, VisibleComponent& visibleComponent)
{
	bindShader();

	glm::mat4 m, t, r, p;
	GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	GLRenderingManager::getInstance().getCameraRotMatrix(r);
	GLRenderingManager::getInstance().getCameraTranslationMatrix(t);
	m = visibleComponent.getParentActor()->caclTransformationMatrix();

	updateUniform("uni_MVP", p * r * t * m);
}


ForwardAmbientShader::ForwardAmbientShader()
{
}

ForwardAmbientShader::~ForwardAmbientShader()
{
}

void ForwardAmbientShader::init()
{
	initProgram();
	addShader(GLShader::VERTEX, "GL3.3/forwardAmbientVertex.sf");
	setAttributeLocation(0, "in_Position");
	setAttributeLocation(1, "in_TexCoord");
	addShader(GLShader::FRAGMENT, "GL3.3/forwardAmbientFragment.sf");
	m_ambientIntensity = 1.0f;
	m_color = glm::vec3(0.75f, 1.0f, 1.0f);
	bindShader();
	updateUniform("uni_Texture", 0);
}

void ForwardAmbientShader::draw(LightComponent& lightComponent, VisibleComponent& visibleComponent)
{
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	bindShader();

	glm::mat4 m, t, r, p;
	GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	GLRenderingManager::getInstance().getCameraRotMatrix(r);
	GLRenderingManager::getInstance().getCameraTranslationMatrix(t);
	m = visibleComponent.getParentActor()->caclTransformationMatrix();

	updateUniform("uni_MVP", p * r * t * m);
	updateUniform("uni_ambientIntensity", glm::vec3(m_ambientIntensity, m_ambientIntensity, m_ambientIntensity));
	updateUniform("uni_color", m_color);
}

void ForwardAmbientShader::setAmbientIntensity(float ambientIntensity)
{
	m_ambientIntensity = ambientIntensity;
}

void ForwardAmbientShader::setColor(glm::vec3 color)
{
	m_color = color;
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
	addShader(GLShader::FRAGMENT, "GL3.3/phongFragment.sf");
	m_ambientIntensity = 1.0f;
	m_ambientColor = glm::vec3(0.75f, 0.85f, 1.0f);
	m_diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	bindShader();
	updateUniform("uni_texture", 0);
}

void PhongShader::draw(LightComponent& lightComponent, VisibleComponent& visibleComponent)
{
	glFrontFace(GL_CCW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	bindShader();

	glm::mat4 m, t, r, p;
	GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	GLRenderingManager::getInstance().getCameraRotMatrix(r);
	GLRenderingManager::getInstance().getCameraTranslationMatrix(t);
	m = visibleComponent.getParentActor()->caclTransformationMatrix();

	glm::vec3 cameraPos;
	GLRenderingManager::getInstance().getCameraPos(cameraPos);

	updateUniform("uni_p", p);
	updateUniform("uni_v", r);
	updateUniform("uni_t", t);
	updateUniform("uni_m", m);

	updateUniform("uni_lightPos", lightComponent.getParentActor()->getTransform()->getPos());
	updateUniform("uni_viewPos", cameraPos);

	updateUniform("uni_ambientIntensity", m_ambientIntensity);
	updateUniform("uni_ambientColor", m_ambientColor);

	updateUniform("uni_specularIntensity", 0.75f);

	updateUniform("uni_diffuseColor", lightComponent.getColor() * lightComponent.getIntensity());		
}

void PhongShader::setAmbientIntensity(float ambientIntensity)
{
	m_ambientIntensity = ambientIntensity;
}

void PhongShader::setAmbientColor(glm::vec3 ambientColor)
{
	m_ambientColor = ambientColor;
}

void PhongShader::setDiffuseColor(glm::vec3 diffuseColor)
{
	m_diffuseColor = diffuseColor;
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
	updateUniform("uni_texture", 0);
}

void BillboardShader::draw(LightComponent& lightComponent, VisibleComponent& visibleComponent)
{
	bindShader();

	glm::mat4 m, t, r, p;

	GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	GLRenderingManager::getInstance().getCameraRotMatrix(r);
	GLRenderingManager::getInstance().getCameraTranslationMatrix(t);
	m = visibleComponent.getParentActor()->caclTransformationMatrix();
	// @TODO: multiply with inverse of camera rotation matrix
	updateUniform("uni_p", p);
	updateUniform("uni_v", r);
	updateUniform("uni_t", t);
	updateUniform("uni_m", m);
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

void SkyboxShader::draw(LightComponent& lightComponent, VisibleComponent& visibleComponent)
{
	glFrontFace(GL_CW);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	bindShader();

	// TODO: fix "looking outside" problem// almost there
	glm::mat4 r, p;
	GLRenderingManager::getInstance().getCameraProjectionMatrix(p);
	GLRenderingManager::getInstance().getCameraRotMatrix(r);

	updateUniform("uni_VP", p * r * -1.0f);
}


GLRenderingManager::GLRenderingManager()
{
}


GLRenderingManager::~GLRenderingManager()
{
}

void GLRenderingManager::render( LightComponent& lightComponent, VisibleComponent& visibleComponent)
{
	switch (visibleComponent.getVisiblilityType())
	{
	case visiblilityType::INVISIBLE: break;
	case visiblilityType::BILLBOARD:
		BillboardShader::getInstance().draw(lightComponent, visibleComponent);
		// update visibleGameEntity's mesh& texture
		visibleComponent.draw();
		break;
	case visiblilityType::STATIC_MESH:
		for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
		{
			m_staticMeshGLShader[i]->draw(lightComponent, visibleComponent);
		}
		// update visibleGameEntity's mesh& texture
		visibleComponent.draw();
		break;
	case visiblilityType::SKYBOX:
		glDepthFunc(GL_LEQUAL);
		SkyboxShader::getInstance().draw(lightComponent, visibleComponent);
		// update visibleGameEntity's mesh& texture
		visibleComponent.draw();
		glDepthFunc(GL_LESS);
		break;
	}
}

void GLRenderingManager::getCameraTranslationMatrix(glm::mat4 & t) const
{
	t = cameraTranslationMatrix;
}

void GLRenderingManager::setCameraTranslationMatrix(const glm::mat4 & t)
{
	cameraTranslationMatrix = t;
}


void GLRenderingManager::getCameraRotMatrix(glm::mat4 & v) const
{
	v = cameraViewMatrix;
}

void GLRenderingManager::setCameraViewMatrix(const glm::mat4 & v)
{
	cameraViewMatrix = v;
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
void GLRenderingManager::initialize()
{
	//m_staticMeshGLShader.emplace_back(&BasicGLShader::getInstance());
	m_staticMeshGLShader.emplace_back(&PhongShader::getInstance());
	//m_staticMeshGLShader.emplace_back(&ForwardAmbientShader::getInstance());

	for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
	{
		m_staticMeshGLShader[i]->init();
	}

	BillboardShader::getInstance().init();
	SkyboxShader::getInstance().init();

	this->setStatus(objectStatus::ALIVE);
	LogManager::getInstance().printLog("GLRenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_TEXTURE_2D);
}

void GLRenderingManager::shutdown()
{
	for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
	{
		m_staticMeshGLShader[i].release();
	}
	this->setStatus(objectStatus::SHUTDOWN);
	LogManager::getInstance().printLog("GLRenderingManager has been shutdown.");
}
