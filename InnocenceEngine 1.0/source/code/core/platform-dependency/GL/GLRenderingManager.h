#pragma once
#include "../../manager/IEventManager.h"
#include "../../manager/AssetManager.h"
#include "../../manager/graphic/GUIManager.h"
#include "../../manager/LogManager.h"
#include "../../interface/IVisibleGameEntity.h"

class GLShader
{
public:
	virtual ~GLShader();

	virtual void init() = 0;
	virtual void update(IVisibleGameEntity* visibleGameEntity) = 0;

protected:
	GLShader();

	enum shaderType
	{
		VERTEX,
		GEOMETRY,
		FRAGMENT
	};

	inline void addShader(shaderType shaderType, const std::string& fileLocation) const;
	inline void setAttributeLocation(int arrtributeLocation, const std::string& arrtributeName) const;
	inline void bindShader() const;

	inline void initProgram();
	inline void addUniform(std::string uniform) const;

	inline void updateUniform(const std::string &uniformName, bool uniformValue) const
	{
		glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), (int)uniformValue);
	}

	inline void updateUniform(const std::string &uniformName, int uniformValue) const
	{
		glUniform1i(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
	}

	inline void updateUniform(const std::string &uniformName, float uniformValue) const
	{
		glUniform1f(glGetUniformLocation(m_program, uniformName.c_str()), uniformValue);
	}

	inline void updateUniform(const std::string &uniformName, const glm::vec2 &uniformValue) const
	{
		glUniform2fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, &uniformValue[0]);
	}

	inline void updateUniform(const std::string &uniformName, float x, float y) const
	{
		glUniform2f(glGetUniformLocation(m_program, uniformName.c_str()), x, y);
	}

	inline void updateUniform(const std::string &uniformName, const glm::vec3& uniformValue) const
	{
		glUniform3fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, &uniformValue[0]);
	}

	inline void updateUniform(const std::string &uniformName, float x, float y, float z) const
	{
		glUniform3f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z);
	}

	inline void updateUniform(const std::string &uniformName, float x, float y, float z, float w)
	{
		glUniform4f(glGetUniformLocation(m_program, uniformName.c_str()), x, y, z, w);
	}

	inline void updateUniform(const std::string &uniformName, const glm::mat4& mat) const
	{
		glUniformMatrix4fv(glGetUniformLocation(m_program, uniformName.c_str()), 1, GL_FALSE, &mat[0][0]);
	}

private:
	inline void attachShader(shaderType shaderType, const std::string& fileContent, int m_program) const;
	inline void compileShader() const;
	inline void detachShader(int shader) const;

	unsigned int m_program;
	//std::map<std::string, int> m_uniforms;
};

class BasicGLShader : public GLShader
{

public:
	~BasicGLShader();

	static BasicGLShader& getInstance()
	{
		static BasicGLShader instance;
		return instance;
	}

	void init() override;
	void update(IVisibleGameEntity* visibleGameEntity) override;

private:
	BasicGLShader();
};

class ForwardAmbientShader : public GLShader
{

public:
	~ForwardAmbientShader();

	static ForwardAmbientShader& getInstance()
	{
		static ForwardAmbientShader instance;
		return instance;
	}

	void init() override;
	void update(IVisibleGameEntity* visibleGameEntity) override;
	void setAmbientIntensity(float ambientIntensity);

private:
	ForwardAmbientShader();
	float m_ambientIntensity;
};

class SkyboxShader : public GLShader
{

public:
	~SkyboxShader();

	static SkyboxShader& getInstance()
	{
		static SkyboxShader instance;
		return instance;
	}

	void init() override;
	void update(IVisibleGameEntity* visibleGameEntity) override;

private:
	SkyboxShader();
};

class GLRenderingManager : public IEventManager
{
public:
	~GLRenderingManager();

	static GLRenderingManager& getInstance()
	{
		static GLRenderingManager instance;
		return instance;
	}

	void render(IVisibleGameEntity* visibleGameEntity) const;
	void finishRender() const;

	void getCameraTranslationMatrix(glm::mat4& t) const;
	void setCameraTranslationMatrix(const glm::mat4& t);
	void getCameraViewMatrix(glm::mat4& v) const;
	void setCameraViewMatrix(const glm::mat4& v);
	void getCameraProjectionMatrix(glm::mat4& p) const;
	void setCameraProjectionMatrix(const glm::mat4& p);

private:
	GLRenderingManager();

	std::vector<std::unique_ptr<GLShader>> m_staticMeshGLShader;
	glm::mat4 cameraTranslationMatrix = glm::mat4();
	glm::mat4 cameraViewMatrix = glm::mat4();
	glm::mat4 cameraProjectionMatrix = glm::mat4();

	void init() override;
	void update() override;
	void shutdown() override;
};