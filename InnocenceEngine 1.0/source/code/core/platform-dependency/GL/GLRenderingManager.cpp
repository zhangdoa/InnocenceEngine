#include "stdafx.h"
#include "GLRenderingManager.h"


GLShader::GLShader()
{
}


GLShader::~GLShader()
{
}

void GLShader::addShader(shaderType shaderType, const std::string& shaderFileName) const
{
	attachShader(shaderType, loadShader(shaderFileName), m_program);
}

inline void GLShader::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName) const
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
	if (glGetAttribLocation(m_program, arrtributeName.c_str()) == 0xFFFFFFFF)
	{
		LogManager::getInstance().printLog("Error: Attribute lost: " + arrtributeName);
	}
}

void GLShader::bindShader() const
{
	glUseProgram(m_program);
}

void GLShader::addUniform(std::string uniform) const
{
	int uniformLocation = glGetUniformLocation(m_program, uniform.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		LogManager::getInstance().printLog("Error: Uniform lost: " + uniform);
	}
	//m_uniforms.emplace(std::pair<std::string, int>(uniform.c_str(), uniformLocation));
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

void GLShader::initProgram()
{
	m_program = glCreateProgram();
}

std::string GLShader::loadShader(const std::string & shaderFileName) const
{
	std::ifstream file;
	file.open(("../res/shaders/" + shaderFileName).c_str());
	std::stringstream shaderStream;
	std::string output;

	shaderStream << file.rdbuf();
	output = shaderStream.str();
	file.close();

	return output;
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

void BasicGLShader::update(IVisibleGameEntity *visibleGameEntity)
{
	bindShader();
	glm::mat4 mvp;
	GLRenderingManager::getInstance().getCamera()->getViewProjectionMatrix(mvp);
	mvp = mvp * visibleGameEntity->getParentActor().caclTransformation();
	updateUniform("uni_MVP", mvp);
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
	updateUniform("uni_Texture", 0);
	m_ambientIntensity = 1.0f;
}

void ForwardAmbientShader::update(IVisibleGameEntity *visibleGameEntity)
{
	bindShader();
	glm::mat4 mvp;
	GLRenderingManager::getInstance().getCamera()->getViewProjectionMatrix(mvp);
	mvp = mvp * visibleGameEntity->getParentActor().caclTransformation();

	updateUniform("uni_MVP", mvp);
	updateUniform("uni_ambientIntensity", glm::vec3(m_ambientIntensity, m_ambientIntensity, m_ambientIntensity));
	updateUniform("uni_color", glm::vec3(1.0f, 1.0f, 1.0f));
}

void ForwardAmbientShader::setAmbientIntensity(float ambientIntensity)
{
	m_ambientIntensity = ambientIntensity;
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
	updateUniform("uni_skybox", 0);
}

void SkyboxShader::update(IVisibleGameEntity * visibleGameEntity)
{
	bindShader();

	// TODO: fix "looking outside" problem// almost there
	glm::mat4 projection;
	GLRenderingManager::getInstance().getCamera()->getProjectionMatrix(projection);

	glm::mat4 view;
	GLRenderingManager::getInstance().getCamera()->getRotationMatrix(view);

	glm::mat4 VP;
	VP = projection * view *  -1.0f;

	updateUniform("uni_VP", VP);
}


GLRenderingManager::GLRenderingManager()
{
}


GLRenderingManager::~GLRenderingManager()
{
}

void GLRenderingManager::render(IVisibleGameEntity * visibleGameEntity) const
{
	// update shader
	switch (visibleGameEntity->getVisibleGameEntityType())
	{
	case IVisibleGameEntity::INVISIBLE: break;
	case IVisibleGameEntity::STATIC_MESH:
		for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
		{
			m_staticMeshGLShader[i]->update(visibleGameEntity);
		}
		break;
	case IVisibleGameEntity::SKYBOX:
		glDepthFunc(GL_LEQUAL);
		SkyboxShader::getInstance().update(visibleGameEntity);
		break;
	}

	// update visibleGameEntity's mesh& texture
	visibleGameEntity->render();
}

void GLRenderingManager::finishRender() const
{
	glDepthFunc(GL_LESS);
}

CameraComponent * GLRenderingManager::getCamera() const
{
	return m_cameraComponent;
}

void GLRenderingManager::setCamera(CameraComponent* cameraComponent)
{

	m_cameraComponent = cameraComponent;
}

void GLRenderingManager::init()
{
	glEnable(GL_DEPTH_TEST);
	m_staticMeshGLShader.emplace_back(&ForwardAmbientShader::getInstance());

	for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
	{
		m_staticMeshGLShader[i]->init();
	}

	SkyboxShader::getInstance().init();

	this->setStatus(INITIALIZIED);
	LogManager::getInstance().printLog("GLRenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_TEXTURE_2D);
}

void GLRenderingManager::shutdown()
{
	for (size_t i = 0; i < m_staticMeshGLShader.size(); i++)
	{
		m_staticMeshGLShader[i].release();
	}
	this->setStatus(UNINITIALIZIED);
	LogManager::getInstance().printLog("GLRenderingManager has been shutdown.");
}
