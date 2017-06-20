#pragma once
#include "stdafx.h"
#include "Util.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "Transform.h" 

class Shader
{
public:
	Shader();
	~Shader();
	void bind();
	void addUniform(std::string uniform);
	void addVertexShaderFromFile(std::string text);
	void addGeometryShaderFromFile(std::string text);
	void addFragmentShaderFromFile(std::string text);
	void addVertexShader(std::string text);
	void addGeometryShader(std::string text);
	void addFragmentShader(std::string text);
	void setAttribLocation(std::string attributeName, int location);
	void compileShader();
	void addProgram(std::string text, int type);
	void setUniformi(std::string uniformName, int value);
	void setUniformf(std::string uniformName, float value);
	void setUniform(std::string uniformName, Vec3f value);
	void setUniform(std::string uniformName, Mat4f value);
private:
	int _program;
	std::map<std::string, int> _uniforms;
	static std::string loadShader(const std::string& fileName);

};
