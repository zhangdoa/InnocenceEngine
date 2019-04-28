#pragma once
#include "../common/InnoType.h"
#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"
#include "../component/GLRenderPassComponent.h"

class GLRenderingSystemComponent
{
public:
	~GLRenderingSystemComponent() {};

	static GLRenderingSystemComponent& get()
	{
		static GLRenderingSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLFrameBufferDesc depthOnlyPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc depthOnlyPassTextureDesc = TextureDataDesc();

	GLFrameBufferDesc deferredPassFBDesc = GLFrameBufferDesc();
	TextureDataDesc deferredPassTextureDesc = TextureDataDesc();

	GLuint m_cameraUBO;
	GLuint m_meshUBO;
	GLuint m_materialUBO;
	GLuint m_sunUBO;
	GLuint m_pointLightUBO;
	GLuint m_sphereLightUBO;
	GLuint m_CSMUBO;
	GLuint m_skyUBO;
	GLuint m_dispatchParamsUBO;

	GLuint m_gridFrustumsSSBO;
	GLuint m_lightIndexListSSBO;
	GLuint m_lightListIndexCounterSSBO;

private:
	GLRenderingSystemComponent() {};
};
