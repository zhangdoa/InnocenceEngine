#include "stdafx.h"
#include "GLRenderingManager.h"


GLShader::GLShader()
{
}


GLShader::~GLShader()
{
}

void GLShader::addShader(shaderType shaderType, const std::string& shaderFileName)
{
	attachShader(shaderType, loadShader(shaderFileName), m_program);
}

void GLShader::bindShader()
{
	glUseProgram(m_program);
}

void GLShader::addUniform(std::string uniform)
{
	int uniformLocation = glGetUniformLocation(m_program, uniform.c_str());
	if (uniformLocation == 0xFFFFFFFF)
	{
		LogManager::printLog("Error : Uniform lost: " + uniform);
	}
	m_uniforms.emplace(std::pair<std::string, int>(uniform.c_str(), uniformLocation));
}

inline void GLShader::attachShader(shaderType shaderType, const std::string& shaderFileContent, int m_program)
{
	int l_glShaderType = 0;

	switch (shaderType)
	{
	case VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: LogManager::printLog("Unknown shader type, cannot add program!");
		break;
	}

	int l_shader = glCreateShader(l_glShaderType);

	if (l_shader == 0) {
		LogManager::printLog("Shader creation failed: memory location invaild when adding shader!");
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
		LogManager::printLog(&ShaderErrorMessage[0]);
	}

	glAttachShader(m_program, l_shader);

	compileShader();
	if (l_glShaderType == GL_VERTEX_SHADER)
	{
		setAttributeLocation(0, "position");
		//addUniform("MVP");
	}
	detachShader(l_shader);
}

inline void GLShader::compileShader()
{
	glLinkProgram(m_program);

	glValidateProgram(m_program);
}

inline void GLShader::setAttributeLocation(int arrtributeLocation, const std::string & arrtributeName)
{
	glBindAttribLocation(m_program, arrtributeLocation, arrtributeName.c_str());
}

inline void GLShader::detachShader(int shader)
{
	glDetachShader(m_program, shader);
	glDeleteShader(shader);
}

void GLShader::initProgram()
{
	m_program = glCreateProgram();
}

std::string GLShader::loadShader(const std::string & shaderFileName)
{
	std::ifstream file;
	file.open(("../res/shaders/" + shaderFileName).c_str());

	std::string output;
	std::string line;

	if (file.is_open())
	{
		while (file.good())
		{
			getline(file, line);

			if (line.find("#include") == std::string::npos)
			{
				output.append(line + "\n");
			}
			else
			{
				std::string includeFileName = GLShader::split(line, ' ')[1];
				includeFileName = includeFileName.substr(1, includeFileName.length() - 2);

				std::string toAppend = loadShader(includeFileName);
				output.append(toAppend + "\n");
			}
		}
	}
	else
	{
		LogManager::LogManager::printLog("Unable to load shader: ");
	}

	return output;
}

std::vector<std::string> GLShader::split(const std::string& data, char marker)
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
	addShader(GLShader::VERTEX, "basicVertex.sf");
	addShader(GLShader::FRAGMENT, "basicFragment.sf");
}

void BasicGLShader::update()
{
}

GLRenderingManager::GLRenderingManager()
{
}


GLRenderingManager::~GLRenderingManager()
{
}

void GLRenderingManager::render(IVisibleGameEntity * visibleGameEntity)
{
	m_basicGLShader.bindShader();
	m_basicGLShader.updateUniform("VP", m_cameraViewProjectionMatrix);
	m_basicGLShader.updateUniform("M", visibleGameEntity->caclTransformation());
}

void GLRenderingManager::setCameraViewProjectionMatrix(const Mat4f & cameraViewProjectionMatrix)
{
	m_cameraViewProjectionMatrix = cameraViewProjectionMatrix;
}

void GLRenderingManager::init()
{
	m_basicGLShader.init();
	m_basicGLShader.addUniform("VP");
	m_basicGLShader.addUniform("M");
	this->setStatus(INITIALIZIED);
	LogManager::LogManager::printLog("GLRenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderingManager::shutdown()
{
	this->setStatus(UNINITIALIZIED);
	LogManager::printLog("GLRenderingManager has been shutdown.");
}

