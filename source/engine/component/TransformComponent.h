#pragma once
#include "../common/InnoType.h"
#include "../common/InnoMath.h"

struct TransformComponent
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN; // 4 Bytes
	EntityID m_parentEntity; // 4 Bytes

	// @TODO: k-d tree?
	unsigned int m_transformHierarchyLevel = 0; // 4 Bytes

	TransformVector m_localTransformVector; // 16 Bytes
	TransformMatrix m_localTransformMatrix; // 64 Bytes

	TransformVector m_globalTransformVector; // 16 Bytes
	TransformMatrix m_globalTransformMatrix; // 64 Bytes

	TransformMatrix m_globalTransformMatrix_prev; // 64 Bytes

	TransformComponent* m_parentTransformComponent = 0; // 4 Bytes in x86, 8 Bytes in x86-64

	// 260 or 264 Bytes at all
};

