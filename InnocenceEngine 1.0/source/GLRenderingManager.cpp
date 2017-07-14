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
	if (l_glShaderType == GL_VERTEX_SHADER)
	{
		setAttributeLocation(0, "in_Position");
		setAttributeLocation(1, "in_TexCoord");
		//setAttributeLocation(2, "in_Normal");
	}
}

inline void GLShader::compileShader() const
{
	glLinkProgram(m_program);

	glValidateProgram(m_program);
}

inline void GLShader::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName) const
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
	if (glGetAttribLocation(m_program, arrtributeName.c_str()) == 0xFFFFFFFF)
	{
		LogManager::getInstance().printLog("Error: Attribute lost: " + arrtributeName);
	}
}

inline void GLShader::detachShader(int shader) const
{
	glDetachShader(m_program, shader);
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

std::vector<std::string> GLShader::split(const std::string& data, char marker) const
{
	std::vector<std::string> elems;

	const char* cstr = data.c_str();
	unsigned int strLength = (unsigned int)data.length();
	unsigned int start = 0;
	unsigned int end = 0;

	while (end <= strLength)
	{
		while (end <= strLength)
		{
			if (cstr[end] == marker)
				break;
			end++;
		}

		elems.push_back(data.substr(start, end - start));
		start = end + 1;
		end = start;
	}

	return elems;
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
	addShader(GLShader::FRAGMENT, "GL3.3/basicFragment.sf");
	updateUniform("uni_Texture", 0);
}

void BasicGLShader::update(IVisibleGameEntity *visibleGameEntity)
{
	bindShader();
	glm::mat4 mvp = GLRenderingManager::getInstance().getCamera()->getViewProjectionMatrix() * visibleGameEntity->getParentActor().caclTransformation();
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
	addShader(GLShader::FRAGMENT, "GL3.3/forwardAmbientFragment.sf");
	updateUniform("uni_Texture", 0);
	m_ambientIntensity = 1.0f;
}

void ForwardAmbientShader::update(IVisibleGameEntity *visibleGameEntity)
{
	bindShader();
	glm::mat4 mvp = GLRenderingManager::getInstance().getCamera()->getViewProjectionMatrix() * visibleGameEntity->getParentActor().caclTransformation();
	updateUniform("uni_MVP", mvp);
	updateUniform("uni_ambientIntensity", glm::vec3(m_ambientIntensity, m_ambientIntensity, m_ambientIntensity));
	updateUniform("uni_color", GUIManager::getInstance().getColor());
}

void ForwardAmbientShader::setAmbientIntensity(float ambientIntensity)
{
	m_ambientIntensity = ambientIntensity;
}

GLRenderingManager::GLRenderingManager()
{
}


GLRenderingManager::~GLRenderingManager()
{
}

void GLRenderingManager::render(IVisibleGameEntity * visibleGameEntity) const
{
	for (size_t i = 0; i < m_GLShader.size(); i++)
	{
		m_GLShader[i]->update(visibleGameEntity);
	}
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
	m_GLShader.emplace_back(&ForwardAmbientShader::getInstance());

	for (size_t i = 0; i < m_GLShader.size(); i++)
	{
		m_GLShader[i]->init();
	}

	this->setStatus(INITIALIZIED);
	LogManager::getInstance().printLog("GLRenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_DEPTH_CLAMP);
	glEnable(GL_TEXTURE_2D);
}

void GLRenderingManager::shutdown()
{
	for (size_t i = 0; i < m_GLShader.size(); i++)
	{
		m_GLShader[i].release();
	}
	this->setStatus(UNINITIALIZIED);
	LogManager::getInstance().printLog("GLRenderingManager has been shutdown.");
}

