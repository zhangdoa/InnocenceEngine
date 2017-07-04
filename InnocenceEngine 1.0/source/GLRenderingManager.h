#pragma once
#include "IEventManager.h"

class Shader
{
public:
	Shader();
	~Shader();

	enum shaderType
	{
		VERTEX,
		GEOMETRY,
		FRAGMENT
	};

	void addShader(shaderType shaderType, std::string fileLocation, int shaderProgram);
	void addProgram(shaderType shaderType, std::string fileContent, int shaderProgram);
	void bind(int shaderProgram);

private:
	std::string loadShader(const std::string& fileName);
	std::vector<std::string> split(const std::string & data, char marker);
};

class GLRenderingManager : public IEventManager
{
	friend Shader;

public:
	GLRenderingManager();
	~GLRenderingManager();

private:
	int m_program;
	GLuint VertexArrayID;
	GLuint vertexbuffer;
	unsigned int VBO, VAO;
	Shader vertexShader;
	Shader fragmentShader;
	void init() override;
	void update() override;
	void shutdown() override;
};