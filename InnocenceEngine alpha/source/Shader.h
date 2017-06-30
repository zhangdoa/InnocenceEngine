#pragma once
#include "stdafx.h"
#include "Math.h"
#include "FragmentPrime.h"

class GameObject; 
class Shader
{
public:
	static std::vector<std::string> split(const std::string & data, char marker);
	Shader();
	~Shader();

	void init();

	void bind();
	void addUniform(std::string uniform);
	void addVertexShader(std::string text);
	void addGeometryShader(std::string text);
	void addFragmentShader(std::string text);
	void setAttribLocation(std::string attributeName, int location);
	void addProgram(std::string text, int type);
	void setUniformi(std::string uniformName, int value);
	void setUniformf(std::string uniformName, float value);
	void setUniform(std::string uniformName,const Vec3f& value);
	void setUniform(std::string uniformName, const Mat4f& value);
	virtual void updateUniforms(GameObject* gameObject, Material* material);
	const std::string& getName();
	void setName(const std::string& name);

private:
	std::string _name;
	int _program;
	std::map<std::string, int> _uniforms;
	static std::string loadShader(const std::string& fileName);
};


class ForwardAmbientShaderWrapper : public Shader
{
public:
	ForwardAmbientShaderWrapper();
	~ForwardAmbientShaderWrapper();

	void updateUniforms(GameObject* gameObject, Material* material) override;
};

class BasicShaderWrapper : public Shader
{
public:
	BasicShaderWrapper();
	~BasicShaderWrapper();

	void updateUniforms(GameObject* gameObject, Material* material) override;
};