#include "stdafx.h"
#include "GLRenderingManager.h"


GLShader::GLShader()
{
}


GLShader::~GLShader()
{
}

void GLShader::addShader(shaderType shaderType, std::string shaderFileName,int shaderProgram)
{
	switch (shaderType)
	{
	case VERTEX: addProgram(VERTEX, loadShader(shaderFileName), shaderProgram);  break;
	case GEOMETRY: addProgram(GEOMETRY, loadShader(shaderFileName), shaderProgram); break;
	case FRAGMENT: addProgram(FRAGMENT, loadShader(shaderFileName), shaderProgram); break;
	default: LogManager::LogManager::printLog("Unknown shader type, cannot add shader!");
		break;
	}
}

void GLShader::addProgram(shaderType shaderType, std::string shaderFileContent, int shaderProgram)
{
	int l_glShaderType = 0;

	switch (shaderType)
	{
	case VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: LogManager::LogManager::printLog("Unknown shader type, cannot add program!");
		break;
	}

	int l_shader = glCreateShader(l_glShaderType);
	if (l_shader == 0) {
		LogManager::LogManager::printLog("Shader creation failed: memory location invaild when adding shader");
	}

	char const * sourcePointer = shaderFileContent.c_str();
	glShaderSource(l_shader, 1, &sourcePointer, NULL);

	GLint Result = GL_FALSE;
	int InfoLogLength;

	glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(shaderProgram, GL_INFO_LOG_LENGTH, &InfoLogLength);

	if (InfoLogLength > 0) {
		std::vector<char> ShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(shaderProgram, InfoLogLength, NULL, &ShaderErrorMessage[0]);
		LogManager::LogManager::printLog(&ShaderErrorMessage[0]);
	}

	glAttachShader(shaderProgram, l_shader);
	glLinkProgram(shaderProgram);

	glValidateProgram(shaderProgram);

	glDetachShader(shaderProgram, l_shader);
	glDeleteShader(l_shader);
}

void GLShader::bind(int shaderProgram)
{
	glUseProgram(shaderProgram);
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


GLRenderingManager::GLRenderingManager()
{
}


GLRenderingManager::~GLRenderingManager()
{
}

void GLRenderingManager::init()
{
	// shader program
	m_program = glCreateProgram();
	if (m_program == 0)
	{
		this->setStatus(ERROR);
		LogManager::LogManager::printLog("Shader creation failed: memory location invaild");
	}

	// vertex shader
	m_vertexShader.addShader(GLShader::VERTEX, "basicVertex.sf", m_program);
	// fragment shader
	m_fragmentShader.addShader(GLShader::FRAGMENT, "basicFragment.sf", m_program);

	this->setStatus(INITIALIZIED);
	LogManager::LogManager::printLog("GLRenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	m_vertexShader.bind(m_program);
	m_fragmentShader.bind(m_program);
}

void GLRenderingManager::shutdown()
{
	this->setStatus(UNINITIALIZIED);
	LogManager::LogManager::printLog("GLRenderingManager has been shutdown.");
}
