#pragma once
#include "TextureDataComponent.h"
#include "../system/GLRenderer/GLHeaders.h"

class GLTextureDataComponent : public TextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	GLuint m_TAO;
};

