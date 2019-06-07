#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMath.h"

struct TransformComponent : public InnoComponent
{
	TransformVector m_localTransformVector;
	TransformMatrix m_localTransformMatrix;

	TransformVector m_globalTransformVector;
	TransformMatrix m_globalTransformMatrix;

	TransformMatrix m_globalTransformMatrix_prev;

	unsigned int m_transformHierarchyLevel = 0;
	TransformComponent* m_parentTransformComponent = 0;
};
