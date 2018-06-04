#pragma once
#include "TextureDataComponent.h"
#include "common/GLHeaders.h"

class GLTextureDataComponent : public TextureDataComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;

	GLuint m_TAO;
};

