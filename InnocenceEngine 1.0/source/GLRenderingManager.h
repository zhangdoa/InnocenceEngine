#pragma once
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

	void addShader(shaderType shaderType, const std::string& fileLocation) const;

	void bindShader() const;

	void addUniform(std::string uniform) const;

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

	void updateUniform(const std::string &uniformName, const glm::vec2 &uniformValue) const
	{
		float bufferUniformValue[] = { uniformValue.x, uniformValue.y };
		glUniform2fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, &bufferUniformValue[0]);
	}

	void updateUniform(const std::string &uniformName, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(m_program, uniformName.c_str()), x, y);
	}

	void updateUniform(const std::string &uniformName, const glm::vec3& uniformValue) const
	{
		float bufferUniformValue[] = { uniformValue.x, uniformValue.y, uniformValue.z };
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

	void updateUniform(const std::string &uniformName, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

	virtual void init() = 0;
	virtual void update(IVisibleGameEntity *visibleGameEntity) = 0;

protected:
	void initProgram();
private:
	std::string loadShader(const std::string& shaderFileName) const;

	inline void attachShader(shaderType shaderType, const std::string& fileContent, int m_program) const;
	inline void compileShader() const;
	inline void setAttributeLocation(int arrtributeLocation, const std::string& arrtributeName) const;
	inline void detachShader(int shader) const;

	std::vector<std::string> split(const std::string& data, char marker);

	unsigned int m_program;
	std::map<std::string, int> m_uniforms;
};

class BasicGLShader : public GLShader
{

public: BasicGLShader();
		~BasicGLShader();

		static BasicGLShader& getInstance()
		{
			static BasicGLShader instance;
			return instance;
		}

		void init() override;
		void update(IVisibleGameEntity *visibleGameEntity) override;
};

class ForwardAmbientShader : public GLShader
{

public: ForwardAmbientShader();
		~ForwardAmbientShader();

		static ForwardAmbientShader& getInstance()
		{
			static ForwardAmbientShader instance;
			return instance;
		}

		void init() override;
		void update(IVisibleGameEntity *visibleGameEntity) override;
		void setAmbientIntensity(float ambientIntensity);
private:
	float m_ambientIntensity;
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

	void render(IVisibleGameEntity* visibleGameEntity) const;

	glm::mat4& getCameraViewProjectionMatrix();
	void setCameraViewProjectionMatrix(const glm::mat4& cameraViewProjectionMatrix);
private:

	void init() override;
	void update() override;
	void shutdown() override;

	glm::mat4 m_cameraViewProjectionMatrix;
	std::vector<std::auto_ptr<GLShader>> m_GLShader;
};