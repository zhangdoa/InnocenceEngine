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
	unsigned int m_transformHierarchyColumnIndex = 0;
	unsigned int m_transformHierarchyRowIndex = 0;

	Transform m_currentTransform;
	Transform m_previousTransform;
};

