#pragma once
#include "BaseComponent.h"
#include "entity/InnoMath.h"

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
	meshID m_AABBMeshID;
	AABB m_AABB;

	std::string m_modelFileName;
	textureFileNameMap m_textureFileNameMap;

	const modelMap& getModelMap() const;
	void setModelMap(const modelMap & modelMap);
	void addMeshData(const meshID & meshID);
	void addTextureData(const texturePair & texturePair);
	void overwriteTextureData(const texturePair & texturePair);

private:
	modelMap m_modelMap;
};

