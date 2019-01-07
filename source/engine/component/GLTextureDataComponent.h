#pragma once
#include "../common/InnoType.h"
#include "../system/GLHeaders.h"

struct GLTextureDataDesc
{
	GLenum textureType;
	GLenum textureWrapMethod;
	GLenum minFilterParam;
	GLenum magFilterParam;
	GLenum internalFormat;
	GLenum pixelDataFormat;
	GLenum pixelDataType;
	float boardColor[4];
};

class GLTextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	GLuint m_TAO = 0;
	GLTextureDataDesc m_GLTextureDataDesc = GLTextureDataDesc();
};

