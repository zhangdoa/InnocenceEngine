#pragma once
#include "../Common/InnoType.h"
#include "../Component/GLMeshDataComponent.h"
#include "../Component/GLTextureDataComponent.h"
#include "../Component/GLRenderPassComponent.h"

class GLRenderingBackendComponent
{
public:
	~GLRenderingBackendComponent() {};

	static GLRenderingBackendComponent& get()
	{
		static GLRenderingBackendComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;

	RenderPassDesc m_deferredRenderPassDesc = RenderPassDesc();

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
	GLRenderingBackendComponent() {};
};
