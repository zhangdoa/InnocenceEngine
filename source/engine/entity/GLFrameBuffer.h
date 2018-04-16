#pragma once
#include "common/GLHeaders.h"
#include "BaseFrameBuffer.h"
#include "ComponentHeaders.h"

class GLFrameBuffer : public BaseFrameBuffer
{
public:
	GLFrameBuffer() {};
	virtual ~GLFrameBuffer() {};

	void initialize() override;
	void update(std::vector<CameraComponent*>& cameraComponents, std::vector<LightComponent*>& lightComponents, std::vector<VisibleComponent*>& visibleComponents, std::unordered_map<EntityID, BaseMesh*>& meshMap, std::unordered_map<EntityID, BaseTexture*>& textureMap, bool cleanColorBuffer, bool cleanDepthBuffer) override;
	void setRenderBufferStorageSize(std::vector<CameraComponent *> & cameraComponents, std::vector<LightComponent *> & lightComponents, std::vector<VisibleComponent *> & visibleComponents, std::unordered_map<EntityID, BaseMesh *> & meshMap, std::unordered_map<EntityID, BaseTexture *> & textureMap) override;
	void activeTexture(int textureIndexInOwnerFrameBuffer, int textureIndexInUserFrameBuffer) override;
	void asReadBuffer() override;
	void asWriteBuffer(const vec2& source, const vec2& dest) override;
	void shutdown() override;

private:
	GLuint m_FBO[2];
	GLuint m_RBO;
	GLenum m_internalformat;
	GLenum m_attachment;
};