#pragma once
#include "../common/InnoType.h"
#include "TextureDataComponent.h"

class EnvironmentCaptureComponent
{
public:
	EnvironmentCaptureComponent() {};
	~EnvironmentCaptureComponent() {};

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;
	unsigned int m_UUID = 0;

	std::string m_cubemapTextureFileName;
	TextureDataComponent* m_TDC = nullptr;
};
