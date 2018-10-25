#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "IRenderingSystem.h"
#include "../../component/GLRenderingSystemSingletonComponent.h"
#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/GLMeshDataComponent.h"
#include "../../component/GLTextureDataComponent.h"
#include "../../component/GLFrameBufferComponent.h"
#include "../../component/GLShaderProgramComponent.h"
#include "../LowLevelSystem/MemorySystem.h"

class GLRenderingSystem : public IRenderingSystem
{
public:
	InnoHighLevelSystem_EXPORT bool setup() override;
	InnoHighLevelSystem_EXPORT bool initialize() override;
	InnoHighLevelSystem_EXPORT bool update() override;
	InnoHighLevelSystem_EXPORT bool terminate() override;

	InnoHighLevelSystem_EXPORT objectStatus getStatus() override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;

	double RadicalInverse(int n, int base);
	void initializeHaltonSampler();
	void initializeEnvironmentRenderPass();
	void initializeShadowRenderPass();
	void initializeGeometryRenderPass();
	void initializeLightRenderPass();
	void initializeFinalRenderPass();

	void initializeSkyPass();
	void initializeTAAPass();
	void initializeBloomExtractPass();
	void initializeBloomBlurPass();
	void initializeMotionBlurPass();
	void initializeBillboardPass();
	void initializeDebuggerPass();
	void initializeFinalBlendPass();

	GLMeshDataComponent* initializeMeshDataComponent(MeshDataComponent* rhs);
	GLTextureDataComponent* initializeTextureDataComponent(TextureDataComponent* rhs);
	void initializeShader(GLuint& shaderProgram, GLuint& shaderID, GLuint shaderType, const std::string& shaderFilePath);

	GLMeshDataComponent* addGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* addGLTextureDataComponent(EntityID rhs);

	GLMeshDataComponent* getGLMeshDataComponent(EntityID rhs);
	GLTextureDataComponent* getGLTextureDataComponent(EntityID rhs);

	void updateEnvironmentRenderPass();
	void updateShadowRenderPass();
	void updateGeometryRenderPass();
	void updateLightRenderPass();
	void updateFinalRenderPass();

	GLuint getUniformLocation(GLuint shaderProgram, const std::string& uniformName);

	void updateUniform(const GLint uniformLocation, bool uniformValue);
	void updateUniform(const GLint uniformLocation, int uniformValue);
	void updateUniform(const GLint uniformLocation, double uniformValue);
	void updateUniform(const GLint uniformLocation, double x, double y);
	void updateUniform(const GLint uniformLocation, double x, double y, double z);
	void updateUniform(const GLint uniformLocation, double x, double y, double z, double w);
	void updateUniform(const GLint uniformLocation, const mat4& mat);

	void attachTextureToFramebuffer(TextureDataComponent* TDC, GLTextureDataComponent* GLTextureDataComponent, GLFrameBufferComponent* GLFrameBufferComponent, int colorAttachmentIndex, int textureIndex, int mipLevel);
	void activateShaderProgram(GLShaderProgramComponent* GLShaderProgramComponent);
	void drawMesh(MeshDataComponent* MDC, GLMeshDataComponent* GLMDC);
	void activateTexture(TextureDataComponent* TDC, GLTextureDataComponent* GLTDC, int activateIndex);
};