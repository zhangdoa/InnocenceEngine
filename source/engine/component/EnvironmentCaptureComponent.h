#pragma once
#include "../common/InnoType.h"
#include "TextureDataComponent.h"

class EnvironmentCaptureComponent
{
public:
	EnvironmentCaptureComponent() {};
	~EnvironmentCaptureComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::string m_cubemapTextureFileName;
	TextureDataComponent* m_TDC = nullptr;
};
