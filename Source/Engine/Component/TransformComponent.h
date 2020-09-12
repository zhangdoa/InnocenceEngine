#pragma once
#include "../Common/InnoObject.h"
#include "../Common/InnoMathHelper.h"

class TransformComponent : public InnoComponent
{
public:
	static uint32_t GetTypeID() { return 1; };
	static char* GetTypeName() { return "TransformComponent"; };

	TransformVector m_localTransformVector;
	TransformVector m_localTransformVector_target;
	TransformVector m_globalTransformVector;
	TransformMatrix m_globalTransformMatrix;

	TransformMatrix m_globalTransformMatrix_prev;

	uint32_t m_transformHierarchyLevel = 0;
	TransformComponent* m_parentTransformComponent = 0;
};
