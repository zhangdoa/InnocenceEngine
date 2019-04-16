#pragma once
#include "../common/InnoType.h"
#include "../system/GLRenderingBackend/GLHeaders.h"

struct GLTextureDataDesc
{
	GLenum textureSamplerType;
	GLenum textureWrapMethod;
	GLenum minFilterParam;
	GLenum magFilterParam;
	GLenum internalFormat;
	GLenum pixelDataFormat;
	GLenum pixelDataType;
	float borderColor[4];
};

class GLTextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_TO = 0;
	GLTextureDataDesc m_GLTextureDataDesc = GLTextureDataDesc();
};
