#pragma once
#include "IEventManager.h"
#include "LogManager.h"


class GLShader
{
public:
	GLShader();
	~GLShader();

	enum shaderType
	{
		VERTEX,
		GEOMETRY,
		FRAGMENT
	};

	void addShader(shaderType shaderType, std::string fileLocation, int shaderProgram);
	void bind(int shaderProgram);

private:
	std::string loadShader(const std::string& fileName);
	void addProgram(shaderType shaderType, std::string fileContent, int shaderProgram);
	std::vector<std::string> split(const std::string & data, char marker);
};

class GLRenderingManager : public IEventManager
{
	friend GLShader;

public:
	GLRenderingManager();
	~GLRenderingManager();

private:
	int m_program;

	GLShader m_vertexShader;
	GLShader m_fragmentShader;

	void init() override;
	void update() override;
	void shutdown() override;
};