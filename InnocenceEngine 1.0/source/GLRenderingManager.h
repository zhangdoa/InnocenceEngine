#pragma once
#include "Math.h"
#include "IEventManager.h"
#include "LogManager.h"
#include "IVisibleGameEntity.h"
#include "CameraComponent.h"

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

	void addShader(shaderType shaderType, const std::string& fileLocation);

	void bindShader();

	void addUniform(std::string uniform);

	void updateUniform(const std::string &uniformName, bool uniformValue) const
	{
		glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), (int)uniformValue);
	}

	void updateUniform(const std::string &uniformName, int uniformValue) const
	{
		glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
	}

	void updateUniform(const std::string &uniformName, float uniformValue) const
	{
		glUniform1f(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
	}

	void updateUniform(const std::string &uniformName, const Vec2f &uniformValue) const
	{
		float bufferUniformValue[] = { uniformValue.getX(), uniformValue.getY() };
		glUniform2fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, &bufferUniformValue[0]);
	}

	void updateUniform(const std::string &uniformName, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(m_program, uniformName.c_str()), x, y);
	}

	void updateUniform(const std::string &uniformName, const Vec3f& uniformValue) const
	{
		float bufferUniformValue[] = { uniformValue.getX(), uniformValue.getY(), uniformValue.getZ() };
		glUniform3fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, &bufferUniformValue[0]);
	}

	void updateUniform(const std::string &uniformName, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z);
	}

	void updateUniform(const std::string &uniformName, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z, w);
	}

	void updateUniform(const std::string &uniformName, const Mat4f& mat) const
	{
		glm::mat4 bufferUniformValue = {
			mat.getElem(0,0), mat.getElem(0,1), mat.getElem(0,2), mat.getElem(0,3),
			mat.getElem(1,0), mat.getElem(1,1), mat.getElem(1,2), mat.getElem(1,3),
			mat.getElem(2,0), mat.getElem(2,1), mat.getElem(2,2), mat.getElem(2,3),
			mat.getElem(3,0), mat.getElem(3,1), mat.getElem(3,2), mat.getElem(3,3) };
		glUniformMatrix4fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, GL_FALSE, &bufferUniformValue[0][0]);
	}

	void updateUniform(const std::string &uniformName, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	virtual void init() = 0;
	virtual void update() = 0;

protected:
	void initProgram();
private:
	std::string loadShader(const std::string& shaderFileName);

	inline void attachShader(shaderType shaderType, const std::string& fileContent, int m_program);
	inline void compileShader();
	inline void setAttributeLocation(int arrtributeLocation, const std::string& arrtributeName);
	inline void detachShader(int shader);

	std::vector<std::string> split(const std::string& data, char marker);

	unsigned int m_program;
	std::map<std::string, int> m_uniforms;
};

class BasicGLShader : public GLShader
{

public: BasicGLShader();
		~BasicGLShader();
		void init() override;
		void update() override;
};

class GLRenderingManager : public IEventManager
{
	friend GLShader;

public:
	GLRenderingManager();
	~GLRenderingManager();

	static GLRenderingManager& getInstance()
	{
		static GLRenderingManager instance;
		return instance;
	}

	void render(IVisibleGameEntity* visibleGameEntity);
	void setCameraViewProjectionMatrix(const Mat4f& cameraViewProjectionMatrix);
private:

	void init() override;
	void update() override;
	void shutdown() override;

	Mat4f m_cameraViewProjectionMatrix;
	BasicGLShader m_basicGLShader;
};