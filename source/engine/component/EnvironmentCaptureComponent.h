#pragma once
#include "../common/InnoType.h"

class EnvironmentCaptureComponent
{
public:
	EnvironmentCaptureComponent() {};
	~EnvironmentCaptureComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::string m_cubemapTextureFileName;
	texturePair m_texturePair;
};
