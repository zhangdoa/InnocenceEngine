#pragma once
#include "TextureDataComponent.h"
#include "../system/HighLevelSystem/GLHeaders.h"

class GLTextureDataComponent : public BaseComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	GLuint m_TAO = 0;
};

