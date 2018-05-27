#pragma once
#include "interface/IRenderingSystem.h"
#include "component/EnvironmentRenderPassSingletonComponent.h"

#include "interface/IRenderingSystem.h"
#include "interface/IMemorySystem.h"
#include "interface/IAssetSystem.h"
#include "interface/IGameSystem.h"

extern IMemorySystem* g_pMemorySystem;
extern IRenderingSystem* g_pRenderingSystem;
extern IAssetSystem* g_pAssetSystem;
extern IGameSystem* g_pGameSystem;

class GLRenderingSystem : public IRenderingSystem
{
public:
	GLRenderingSystem() {};
	~GLRenderingSystem() {};

	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	const objectStatus& getStatus() const override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	void setupShader(GLuint shaderProgram, GLuint shaderID, GLuint shaderType, const std::string& shaderFilePath);
	GLuint getUniformLocation(GLuint shaderProgram, const std::string& uniformName);
	void updateUniform(const GLint uniformLocation, bool uniformValue) const;
	void updateUniform(const GLint uniformLocation, int uniformValue) const;
	void updateUniform(const GLint uniformLocation, double uniformValue) const;
	void updateUniform(const GLint uniformLocation, double x, double y) const;
	void updateUniform(const GLint uniformLocation, double x, double y, double z) const;
	void updateUniform(const GLint uniformLocation, double x, double y, double z, double w) const;
	void updateUniform(const GLint uniformLocation, const mat4& mat) const;
};
