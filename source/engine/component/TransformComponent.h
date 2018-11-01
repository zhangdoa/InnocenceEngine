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

	// @TODO: k-d tree?
	unsigned int m_transformHierarchyLevel = 0;

	TransformComponent* m_parentTransformComponent = 0;

	TransformVector m_localTransformVector;
	TransformMatrix m_localTransformMatrix;

	TransformVector m_globalTransformVector;
	TransformMatrix m_globalTransformMatrix;
};

