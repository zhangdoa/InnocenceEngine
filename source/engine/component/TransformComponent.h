#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

struct TransformComponent
{
	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_parentEntity;
	unsigned int m_UUID = 0;

	unsigned int m_transformHierarchyLevel = 0;

	TransformVector m_localTransformVector;
	TransformMatrix m_localTransformMatrix;

	TransformVector m_globalTransformVector;
	TransformMatrix m_globalTransformMatrix;

	TransformMatrix m_globalTransformMatrix_prev;

	TransformComponent* m_parentTransformComponent = 0;
};
