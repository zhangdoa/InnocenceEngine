#pragma once
#include "TextureDataComponent.h"
#include "../common/GLHeaders.h"

class GLTextureDataComponent : public TextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	GLuint m_TAO;
};

