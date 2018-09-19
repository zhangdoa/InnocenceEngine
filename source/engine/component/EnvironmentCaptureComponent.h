#pragma once
#include "BaseComponent.h"

class EnvironmentCaptureComponent : public BaseComponent
{
public:
	EnvironmentCaptureComponent() {};
	~EnvironmentCaptureComponent() {};

	std::string m_cubemapTextureFileName;
	texturePair m_texturePair;
};
