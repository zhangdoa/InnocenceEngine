#pragma once
#include "BaseComponent.h"

class VisibleComponent : public BaseComponent
{
public:
	VisibleComponent() {};
	~VisibleComponent() {};

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshShapeType m_meshShapeType = meshShapeType::QUAD;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;

	bool m_caclNormal = false;
	bool m_useTexture = true;
	vec4 m_albedo;
	vec4 m_MRA;
	bool m_drawAABB = false;
	meshID m_AABBMeshID = 0;
	AABB m_AABB;

	std::string m_modelFileName;

	modelMap m_modelMap;
};

