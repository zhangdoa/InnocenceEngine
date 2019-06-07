#pragma once
#include "../Common/InnoComponent.h"
#include "TextureDataComponent.h"

class EnvironmentCaptureComponent : public InnoComponent
{
public:
	EnvironmentCaptureComponent() {};
	~EnvironmentCaptureComponent() {};

	std::string m_cubemapTextureFileName;
	TextureDataComponent* m_TDC = nullptr;
};
