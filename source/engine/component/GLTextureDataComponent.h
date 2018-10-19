#pragma once
#include "TextureDataComponent.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLTextureDataComponent : public TextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	GLuint m_TAO = 0;
};

