#pragma once
#include "../Common/InnoComponent.h"
#include "../Common/InnoMathHelper.h"

class TransformComponent : public InnoComponent
{
public:
	TransformVector m_localTransformVector;
	TransformVector m_localTransformVector_target;
	TransformVector m_globalTransformVector;
	TransformMatrix m_globalTransformMatrix;

	TransformMatrix m_globalTransformMatrix_prev;

	unsigned int m_transformHierarchyLevel = 0;
	TransformComponent* m_parentTransformComponent = 0;
};
