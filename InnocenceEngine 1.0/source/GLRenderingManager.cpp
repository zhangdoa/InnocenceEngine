#include "stdafx.h"
#include "GLRenderingManager.h"


Shader::Shader()
{
}


Shader::~Shader()
{
}

void Shader::addShader(shaderType shaderType, std::string shaderFileName,int shaderProgram)
{
	switch (shaderType)
	{
	case VERTEX: addProgram(VERTEX, loadShader(shaderFileName), shaderProgram);  break;
	case GEOMETRY: addProgram(GEOMETRY, loadShader(shaderFileName), shaderProgram); break;
	case FRAGMENT: addProgram(FRAGMENT, loadShader(shaderFileName), shaderProgram); break;
	default: IEventManager::printLog("Unknown shader type, cannot add shader!");
		break;
	}
}

void Shader::addProgram(shaderType shaderType, std::string shaderFileContent, int shaderProgram)
{
	int l_glShaderType = 0;

	switch (shaderType)
	{
	case VERTEX: l_glShaderType = GL_VERTEX_SHADER;  break;
	case GEOMETRY: l_glShaderType = GL_GEOMETRY_SHADER;  break;
	case FRAGMENT: l_glShaderType = GL_FRAGMENT_SHADER;  break;
	default: IEventManager::printLog("Unknown shader type, cannot add program!");
		break;
	}

	int l_shader = glCreateShader(l_glShaderType);
	if (l_shader == 0) {
		IEventManager::printLog("Shader creation failed: memory location invaild when adding shader");
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
		IEventManager::printLog(&ShaderErrorMessage[0]);
	}

	glAttachShader(shaderProgram, l_shader);
	glLinkProgram(shaderProgram);

	glValidateProgram(shaderProgram);

	glDetachShader(shaderProgram, l_shader);
	glDeleteShader(l_shader);
}

void Shader::bind(int shaderProgram)
{
	glUseProgram(shaderProgram);
}

std::string Shader::loadShader(const std::string & shaderFileName)
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
				std::string includeFileName = Shader::split(line, ' ')[1];
				includeFileName = includeFileName.substr(1, includeFileName.length() - 2);

				std::string toAppend = loadShader(includeFileName);
				output.append(toAppend + "\n");
			}
		}
	}
	else
	{
		IEventManager::printLog("Unable to load shader: ");
	}

	return output;
}


std::vector<std::string> Shader::split(const std::string& data, char marker)
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
		printLog("Shader creation failed: memory location invaild");
	}

	// vertex shader
	vertexShader.addShader(Shader::VERTEX, "basicVertex.sf", m_program);
	// fragment shader
	fragmentShader.addShader(Shader::FRAGMENT, "basicFragment.sf", m_program);

	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// An array of 3 vectors which represents 3 vertices
	GLfloat g_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	printLog("RenderingManager has been initialized.");
}

void GLRenderingManager::update()
{
	// render
	// ------
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	vertexShader.bind(m_program);
	fragmentShader.bind(m_program);
	// draw our first triangle

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void GLRenderingManager::shutdown()
{
	// optional: de-allocate all resources once they've outlived their purpose:
	// ------------------------------------------------------------------------
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);

	printLog("RenderingManager has been shutdown.");
}
