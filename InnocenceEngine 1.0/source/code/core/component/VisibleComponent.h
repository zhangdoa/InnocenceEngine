#pragma once
#include "../interface/BaseEntity.h"
#include "../data/GraphicData.h"

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

	modelMap& getModelMap();
	void setModelMap(modelMap& graphicDataMap);
	void addMeshData(meshID& meshDataID);
	void addTextureData(texturePair& textureDataPair);
	void overwriteTextureData(texturePair& textureDataPair);

private:
	modelMap m_modelMap;
};

