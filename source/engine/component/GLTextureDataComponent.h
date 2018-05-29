#pragma once
#include "BaseComponent.h"
#include "common/GLHeaders.h"

class GLTextureDataComponent : public BaseComponent
{
public:
	GLTextureDataComponent() {};
	~GLTextureDataComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;

	textureID m_textureID;
	textureType m_textureType;

	GLuint m_TAO;
};

