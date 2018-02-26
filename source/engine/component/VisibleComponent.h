#pragma once
#include "entity/BaseEntity.h"
#include "entity/BaseGraphicPrimitive.h"

class VisibleComponent : public BaseComponent
{
public:
	VisibleComponent();
	~VisibleComponent();


	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

	visiblilityType m_visiblilityType = visiblilityType::INVISIBLE;
	meshDrawMethod m_meshDrawMethod = meshDrawMethod::TRIANGLE;
	textureWrapMethod m_textureWrapMethod = textureWrapMethod::REPEAT;
	
	bool m_useTexture = true;
	vec3 m_albedo;
	vec3 m_MRA;

	modelMap& getModelMap();
	void setModelMap(modelMap& graphicDataMap);
	void addMeshData(meshID& meshDataID);
	void addTextureData(texturePair& textureDataPair);
	void overwriteTextureData(texturePair& textureDataPair);

private:
	modelMap m_modelMap;
};

