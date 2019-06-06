#pragma once
#include "../common/InnoComponent.h"
#include "TextureDataComponent.h"

class EnvironmentCaptureComponent : public InnoComponent
{
public:
	EnvironmentCaptureComponent() {};
	~EnvironmentCaptureComponent() {};

	std::string m_cubemapTextureFileName;
	TextureDataComponent* m_TDC = nullptr;
};
