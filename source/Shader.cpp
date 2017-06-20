#include "Shader.h"



Shader::Shader()
{
	_program = glCreateProgram();
	if (_program == 0)
	{
		std::cout << "Shader creation failed: memory location invaild" << std::endl;
	}
}


Shader::~Shader()
{
}

void Shader::bind()
{
	glUseProgram(_program);
}

void Shader::addUniform(std::string uniform)
{
	int uniformLocation = glGetUniformLocation(_program, uniform.data());

	if (uniformLocation == 0xFFFFFFFF) {
		std::cout << "Error : Uniform lost: " + uniform << std::endl;
	}

	_uniforms.insert(std::pair<std::string, int>(uniform, uniformLocation));
}


void Shader::addVertexShaderFromFile(std::string text)
{
	addProgram(loadShader(text), GL_VERTEX_SHADER);
}

void Shader::addGeometryShaderFromFile(std::string text)
{
	addProgram(loadShader(text), GL_GEOMETRY_SHADER);
}

void Shader::addFragmentShaderFromFile(std::string text)
{
	addProgram(loadShader(text), GL_FRAGMENT_SHADER);
}

void Shader::addVertexShader(std::string text)
{
	addProgram(text, GL_VERTEX_SHADER);
}

void Shader::addGeometryShader(std::string text)
{
	addProgram(text, GL_GEOMETRY_SHADER);
}

void Shader::addFragmentShader(std::string text)
{
	addProgram(text, GL_FRAGMENT_SHADER);
}

void Shader::setAttribLocation(std::string attributeName, int location)
{
	glBindAttribLocation(_program, location, attributeName.data());
}

void Shader::compileShader()
{
	glLinkProgram(_program);
	glValidateProgram(_program);
}

void Shader::addProgram(std::string text, int type)
{
	int shader = glCreateShader(type);
	if (shader == 0) {
		std::cout << "Shader creation failed: memory location invaild when adding shader" << std::endl;
	}

	//glShaderSource(shader, text.size(), text.data());
	glCompileShader(shader);
}

void Shader::setUniformi(std::string uniformName, int value)
{
}

void Shader::setUniformf(std::string uniformName, float value)
{
}

void Shader::setUniform(std::string uniformName, Vec3f value)
{
}

void Shader::setUniform(std::string uniformName, Mat4f value)
{
}

std::string Shader::loadShader(const std::string & fileName)
{
	std::ifstream file;
	file.open(("./res/shaders/" + fileName).c_str());

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
				std::string includeFileName = Util::split(line, ' ')[1];
				includeFileName = includeFileName.substr(1, includeFileName.length() - 2);

				std::string toAppend = loadShader(includeFileName);
				output.append(toAppend + "\n");
			}
		}
	}
	else
	{
		std::cerr << "Unable to load shader: " << fileName << std::endl;
	}

	return output;
}
