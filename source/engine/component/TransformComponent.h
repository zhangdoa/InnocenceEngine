#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

class TransformComponent
{
public:
	TransformComponent() {};
	~TransformComponent() {};

	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	Transform m_currentTransform;
	Transform m_previousTransform;
};

